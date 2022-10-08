#include "trc/core/SceneBase.h"



trc::SceneBase::UniqueDrawableRegistrationId::UniqueDrawableRegistrationId(
    DrawableExecutionRegistration::ID id,
    SceneBase& scene)
    :
    id(id),
    scene(&scene)
{
}

trc::SceneBase::UniqueDrawableRegistrationId::UniqueDrawableRegistrationId(UniqueDrawableRegistrationId&& other) noexcept
    :
    id(other.id),
    scene(other.scene)
{
    other.scene = nullptr;
}

trc::SceneBase::UniqueDrawableRegistrationId::~UniqueDrawableRegistrationId()
{
    if (scene != nullptr) {
        scene->unregisterDrawFunction(id);
    }
}

auto trc::SceneBase::UniqueDrawableRegistrationId::operator=(UniqueDrawableRegistrationId&& other) noexcept
    -> UniqueDrawableRegistrationId&
{
    std::swap(id, other.id);
    scene = other.scene;
    other.scene = nullptr;

    return *this;
}



trc::SceneBase::DrawableExecutionRegistration::DrawableExecutionRegistration(
    std::unique_ptr<RegistrationIndex> indexStruct,
    DrawableFunction func)
    :
    indexInRegistrationArray(std::move(indexStruct)), recordFunction(func)
{}

trc::SceneBase::DrawableExecutionRegistration::RegistrationIndex::RegistrationIndex(
    RenderStage::ID stage,
    SubPass::ID sub,
    Pipeline::ID pipeline,
    ui32 i)
    :
    renderStage(stage),
    subPass(sub),
    pipeline(pipeline),
    indexInRegistrationArray(i)
{
}



auto trc::SceneBase::PipelineListProxy::begin() const -> const_iterator
{
    return pipelines.begin();
}

auto trc::SceneBase::PipelineListProxy::end() const -> const_iterator
{
    return pipelines.end();
}

bool trc::SceneBase::PipelineListProxy::empty() const
{
    return pipelines.empty();
}

auto trc::SceneBase::PipelineListProxy::size() const -> size_t
{
    return pipelines.size();
}



template<typename T>
auto trc::SceneBase::LockedStorage<T>::read() const
    -> std::pair<const T&, std::shared_lock<std::shared_mutex>>
{
    return { *data, std::shared_lock(*mutex) };
}

template<typename T>
auto trc::SceneBase::LockedStorage<T>::write()
    -> std::pair<T&, std::unique_lock<std::shared_mutex>>
{
    return { *data, std::unique_lock(*mutex) };
}

auto trc::SceneBase::readDrawCalls(
    RenderStage::ID renderStage,
    SubPass::ID subPass,
    Pipeline::ID pipelineId) const
    -> std::pair<
        const std::vector<DrawableExecutionRegistration>&,
        std::shared_lock<std::shared_mutex>
    >
{
    std::shared_lock lock(drawRegsMutex);
    auto [drawCalls, _3] = drawRegistrations[renderStage][subPass][pipelineId].read();

    return std::pair<
        const std::vector<DrawableExecutionRegistration>&,
        std::shared_lock<std::shared_mutex>
    >{ drawCalls, std::move(_3) };
}

auto trc::SceneBase::writeDrawCalls(
    RenderStage::ID renderStage,
    SubPass::ID subPass,
    Pipeline::ID pipelineId)
    -> std::pair<
        std::vector<DrawableExecutionRegistration>&,
        std::unique_lock<std::shared_mutex>
    >
{
    std::scoped_lock lock(drawRegsMutex);
    auto [drawCalls, _3] = drawRegistrations[renderStage][subPass][pipelineId].write();

    return std::pair<
        std::vector<DrawableExecutionRegistration>&,
        std::unique_lock<std::shared_mutex>
    >{ drawCalls, std::move(_3) };
}

auto trc::SceneBase::iterPipelines(
    RenderStage::ID renderStage,
    SubPass::ID subPass
    ) const noexcept -> PipelineListProxy
{
    {
        std::shared_lock lock(uniquePipelinesMutex);
        if (uniquePipelines.size() <= renderStage
            || uniquePipelines[renderStage].size() <= subPass)
        {
            static std::vector<Pipeline::ID> emptyResult;
            return { {}, emptyResult };
        }
    }

    std::shared_lock lock(uniquePipelinesVectorMutex);
    return { std::move(lock), uniquePipelinesVector[renderStage][subPass] };
}

void trc::SceneBase::invokeDrawFunctions(
    RenderStage::ID renderStage,
    RenderPass& renderPass,
    SubPass::ID subPass,
    Pipeline::ID pipelineId,
    Pipeline& pipeline,
    vk::CommandBuffer cmdBuf) const
{
    DrawEnvironment env{
        .currentRenderStage = renderStage,
        .currentRenderPass = &renderPass,
        .currentSubPass = subPass,
        .currentPipeline = &pipeline,
    };

    auto [drawCalls, _] = readDrawCalls(renderStage, subPass, pipelineId);

    for (auto& f : drawCalls)
    {
        f.recordFunction(env, cmdBuf);
    }
}

auto trc::SceneBase::registerDrawFunction(
    RenderStage::ID stage,
    SubPass::ID subPass,
    Pipeline::ID pipeline,
    DrawableFunction commandBufferRecordingFunction
    ) -> MaybeUniqueRegistrationId
{
    tryInsertPipeline(stage, subPass, pipeline);

    auto [currentRegistrationArray, _] = writeDrawCalls(stage, subPass, pipeline);
    auto& reg = currentRegistrationArray.emplace_back(
        std::make_unique<DrawableExecutionRegistration::RegistrationIndex>(
            stage, subPass, pipeline, currentRegistrationArray.size()
        ),
        std::move(commandBufferRecordingFunction)
    );

    return { DrawableExecutionRegistration::ID(reg), *this };
}

void trc::SceneBase::unregisterDrawFunction(RegistrationID id)
{
    if (id.regIndex == nullptr) return;

    const auto [stage, subPass, pipeline, index] = *id.regIndex;

    auto [vectorToRemoveFrom, _] = writeDrawCalls(stage, subPass, pipeline);
    std::swap(vectorToRemoveFrom[index], vectorToRemoveFrom.back());

    // Set new index on registration that has been moved from the back of the vector
    auto& movedElem = vectorToRemoveFrom[index];
    movedElem.indexInRegistrationArray->indexInRegistrationArray = index;

    // Remove old registration that's now at the back of the vector
    vectorToRemoveFrom.pop_back();
    if (vectorToRemoveFrom.empty()) {
        removePipeline(stage, subPass, pipeline);
    }
}

void trc::SceneBase::tryInsertPipeline(
    RenderStage::ID stage,
    SubPass::ID subPass,
    Pipeline::ID pipeline)
{
    std::scoped_lock lock(uniquePipelinesMutex);
    auto [it, success] = uniquePipelines[stage][subPass].insert(pipeline);
    if (success)
    {
        std::scoped_lock lock_(uniquePipelinesVectorMutex);
        uniquePipelinesVector[stage][subPass].push_back(pipeline);
    }
}

void trc::SceneBase::removePipeline(
    RenderStage::ID stage,
    SubPass::ID subPass,
    Pipeline::ID pipeline)
{
    std::scoped_lock lock(uniquePipelinesMutex, uniquePipelinesVectorMutex);
    uniquePipelines[stage][subPass].erase(pipeline);
    uniquePipelinesVector[stage][subPass].erase(std::ranges::find(
        uniquePipelinesVector[stage][subPass], pipeline
    ));
}
