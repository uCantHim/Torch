#include "base/SceneBase.h"



trc::SceneBase::DrawableExecutionRegistration::DrawableExecutionRegistration(
    std::unique_ptr<RegistrationIndex> indexStruct,
    DrawableFunction func)
    :
    indexInRegistrationArray(std::move(indexStruct)), recordFunction(func)
{}



auto trc::SceneBase::getPipelines(
    RenderStageType::ID renderStageType,
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
    RenderStageType::ID renderStageType,
    RenderPass::ID renderPass,
    SubPass::ID subPass,
    Pipeline::ID pipeline,
    vk::CommandBuffer cmdBuf) const
{
    DrawEnvironment env{
        .currentRenderStageType = &RenderStageType::at(renderStageType),
        .currentRenderPass = &RenderPass::at(renderPass),
        .currentSubPass = subPass,
        .currentPipeline = &Pipeline::at(pipeline)
    };

    for (auto& f : drawableRegistrations[renderStageType][subPass][pipeline])
    {
        f.recordFunction(env, cmdBuf);
    }
}

auto trc::SceneBase::registerDrawFunction(
    RenderStageType::ID renderStageType,
    SubPass::ID subPass,
    Pipeline::ID pipeline,
    DrawableFunction commandBufferRecordingFunction
    ) -> RegistrationID
{
    assert(RenderStageType::at(renderStageType).numSubPasses > subPass);

    tryInsertPipeline(renderStageType, subPass, pipeline);

    auto& currentRegistrationArray = drawableRegistrations[renderStageType][subPass][pipeline];
    auto& reg = currentRegistrationArray.emplace_back(
        std::make_unique<DrawableExecutionRegistration::RegistrationIndex>(
            renderStageType, subPass, pipeline, currentRegistrationArray.size()
        ),
        std::move(commandBufferRecordingFunction)
    );

    return { reg };
}

void trc::SceneBase::unregisterDrawFunction(RegistrationID id)
{
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
    RenderStageType::ID renderStageType,
    SubPass::ID subPass,
    Pipeline::ID pipeline)
{
    auto [it, success] = uniquePipelines[renderStageType][subPass].insert(pipeline);
    if (success) {
        uniquePipelinesVector[renderStageType][subPass].push_back(pipeline);
    }
}

void trc::SceneBase::removePipeline(
    RenderStageType::ID renderStageType,
    SubPass::ID subPass,
    Pipeline::ID pipeline)
{
    uniquePipelines[renderStageType][subPass].erase(pipeline);
    uniquePipelinesVector[renderStageType][subPass].erase(std::ranges::find(
        uniquePipelinesVector[renderStageType][subPass], pipeline
    ));
}
