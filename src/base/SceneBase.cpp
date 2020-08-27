#include "base/SceneBase.h"



trc::SceneBase::DrawableExecutionRegistration::DrawableExecutionRegistration(
    std::unique_ptr<RegistrationIndex> indexStruct,
    DrawableFunction func)
    :
    indexInRegistrationArray(std::move(indexStruct)), recordFunction(func)
{}



auto trc::SceneBase::getPipelines(RenderStage::ID renderStage, SubPass::ID subPass) const noexcept
    -> const std::set<Pipeline::ID>&
{
    static std::set<Pipeline::ID> emptyResult;

    if (uniquePipelines.size() <= renderStage
        || uniquePipelines[renderStage].size() <= subPass)
    {
        return emptyResult;
    }

    return uniquePipelines[renderStage][subPass];
}

void trc::SceneBase::invokeDrawFunctions(
    RenderStage::ID renderStage,
    RenderPass::ID renderPass,
    SubPass::ID subPass,
    Pipeline::ID pipeline,
    vk::CommandBuffer cmdBuf) const
{
    DrawEnvironment env{
        .currentRenderStage = &RenderStage::at(renderStage),
        .currentRenderPass = &RenderPass::at(renderPass),
        .currentSubPass = subPass,
        .currentPipeline = &Pipeline::at(pipeline)
    };

    for (auto& f : drawableRegistrations[renderStage][subPass][pipeline])
    {
        f.recordFunction(env, cmdBuf);
    }
}

auto trc::SceneBase::registerDrawFunction(
    RenderStage::ID renderStage,
    SubPass::ID subPass,
    Pipeline::ID pipeline,
    DrawableFunction commandBufferRecordingFunction
    ) -> RegistrationID
{
    assert(RenderStage::at(renderStage).getNumSubPasses() > subPass);

    tryInsertPipeline(renderStage, subPass, pipeline);

    auto& currentRegistrationArray = drawableRegistrations[renderStage][subPass][pipeline];
    auto& reg = currentRegistrationArray.emplace_back(
        std::make_unique<DrawableExecutionRegistration::RegistrationIndex>(
            renderStage, subPass, pipeline, currentRegistrationArray.size()
        ),
        std::move(commandBufferRecordingFunction)
    );

    return { reg };
}

void trc::SceneBase::unregisterDrawFunction(RegistrationID id)
{
    const auto [renderStage, subPass, pipeline, index] = *id.regIndex;

    auto& vectorToRemoveFrom = drawableRegistrations[renderStage][subPass][pipeline];
    std::swap(vectorToRemoveFrom[index], vectorToRemoveFrom.back());

    // Set new index on registration that has been moved from the back of the vector
    auto& movedElem = vectorToRemoveFrom[index];
    movedElem.indexInRegistrationArray->indexInRegistrationArray = index;

    // Remove old registration that's now at the back of the vector
    vectorToRemoveFrom.pop_back();
    if (vectorToRemoveFrom.empty()) {
        removePipeline(renderStage, subPass, pipeline);
    }
}

void trc::SceneBase::tryInsertPipeline(
    RenderStage::ID renderStage,
    SubPass::ID subPass,
    Pipeline::ID pipeline)
{
    auto [it, success] = uniquePipelines[renderStage][subPass].insert(pipeline);
    if (success) {
        uniquePipelinesVector[renderStage][subPass].push_back(pipeline);
    }
}

void trc::SceneBase::removePipeline(
    RenderStage::ID renderStage,
    SubPass::ID subPass,
    Pipeline::ID pipeline)
{
    uniquePipelines[renderStage][subPass].erase(pipeline);
    uniquePipelinesVector[renderStage][subPass].erase(std::ranges::find(
        uniquePipelinesVector[renderStage][subPass], pipeline
    ));
}
