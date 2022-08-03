#include "core/SceneBase.h"



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
    renderStageType(stage),
    subPass(sub),
    pipeline(pipeline),
    indexInRegistrationArray(i)
{
}



auto trc::SceneBase::getPipelines(
    RenderStage::ID renderStageType,
    SubPass::ID subPass
    ) const noexcept -> const std::vector<Pipeline::ID>&
{
    static std::vector<Pipeline::ID> emptyResult;

    if (uniquePipelines.size() <= renderStageType
        || uniquePipelines[renderStageType].size() <= subPass)
    {
        return emptyResult;
    }

    return uniquePipelinesVector[renderStageType][subPass];
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

    for (auto& f : drawableRegistrations[renderStage][subPass][pipelineId])
    {
        f.recordFunction(env, cmdBuf);
    }
}

auto trc::SceneBase::registerDrawFunction(
    RenderStage::ID renderStageType,
    SubPass::ID subPass,
    Pipeline::ID pipeline,
    DrawableFunction commandBufferRecordingFunction
    ) -> MaybeUniqueRegistrationId
{
    tryInsertPipeline(renderStageType, subPass, pipeline);

    auto& currentRegistrationArray = drawableRegistrations[renderStageType][subPass][pipeline];
    auto& reg = currentRegistrationArray.emplace_back(
        std::make_unique<DrawableExecutionRegistration::RegistrationIndex>(
            renderStageType, subPass, pipeline, currentRegistrationArray.size()
        ),
        std::move(commandBufferRecordingFunction)
    );

    return { DrawableExecutionRegistration::ID(reg), *this };
}

void trc::SceneBase::unregisterDrawFunction(RegistrationID id)
{
    if (id.regIndex == nullptr) return;

    const auto [renderStageType, subPass, pipeline, index] = *id.regIndex;

    auto& vectorToRemoveFrom = drawableRegistrations[renderStageType][subPass][pipeline];
    std::swap(vectorToRemoveFrom[index], vectorToRemoveFrom.back());

    // Set new index on registration that has been moved from the back of the vector
    auto& movedElem = vectorToRemoveFrom[index];
    movedElem.indexInRegistrationArray->indexInRegistrationArray = index;

    // Remove old registration that's now at the back of the vector
    vectorToRemoveFrom.pop_back();
    if (vectorToRemoveFrom.empty()) {
        removePipeline(renderStageType, subPass, pipeline);
    }
}

void trc::SceneBase::tryInsertPipeline(
    RenderStage::ID renderStageType,
    SubPass::ID subPass,
    Pipeline::ID pipeline)
{
    auto [it, success] = uniquePipelines[renderStageType][subPass].insert(pipeline);
    if (success) {
        uniquePipelinesVector[renderStageType][subPass].push_back(pipeline);
    }
}

void trc::SceneBase::removePipeline(
    RenderStage::ID renderStageType,
    SubPass::ID subPass,
    Pipeline::ID pipeline)
{
    uniquePipelines[renderStageType][subPass].erase(pipeline);
    uniquePipelinesVector[renderStageType][subPass].erase(std::ranges::find(
        uniquePipelinesVector[renderStageType][subPass], pipeline
    ));
}
