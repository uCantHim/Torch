#include "base/SceneBase.h"



trc::SceneBase::DrawableExecutionRegistration::DrawableExecutionRegistration(
    RenderStage::ID stage,
    SubPass::ID subPass,
    GraphicsPipeline::ID pipeline,
    DrawableFunction func)
    :
    renderStage(stage), subPass(subPass), pipeline(pipeline), recordFunction(func)
{}



auto trc::SceneBase::getPipelines(RenderStage::ID renderStage, SubPass::ID subPass) const noexcept
    -> const std::set<GraphicsPipeline::ID>&
{
    static std::set<GraphicsPipeline::ID> emptyResult;

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
    GraphicsPipeline::ID pipeline,
    vk::CommandBuffer cmdBuf) const
{
    DrawEnvironment env{
        .currentRenderStage = &RenderStage::at(renderStage),
        .currentRenderPass = &RenderPass::at(renderPass),
        .currentSubPass = subPass,
        .currentPipeline = &GraphicsPipeline::at(pipeline)
    };

    for (auto& f : drawableRegistrations[renderStage][subPass][pipeline])
    {
        f.recordFunction(env, cmdBuf);
    }
}

auto trc::SceneBase::registerDrawFunction(
    RenderStage::ID renderStage,
    SubPass::ID subpass,
    GraphicsPipeline::ID usedPipeline,
    DrawableFunction commandBufferRecordingFunction
    ) -> RegistrationID
{
    tryInsertPipeline(renderStage, subpass, usedPipeline);

    return insertRegistration(
        { renderStage, subpass, usedPipeline, std::move(commandBufferRecordingFunction) }
    );
}

void trc::SceneBase::unregisterDrawFunction(RegistrationID id)
{
    DrawableExecutionRegistration* entry = *id.reg;

    const auto renderStage = entry->renderStage;
    const auto subPass = entry->subPass;
    const auto pipeline = entry->pipeline;
    const auto index = entry->indexInRegistrationArray;

    auto& vectorToRemoveFrom = drawableRegistrations[renderStage][subPass][pipeline];

    auto& movedElem = vectorToRemoveFrom.back();
    std::swap(vectorToRemoveFrom[index], movedElem);

    // Set backreferencing values
    *movedElem.thisPointer = &movedElem;
    movedElem.indexInRegistrationArray = index;

    vectorToRemoveFrom.pop_back();
    if (vectorToRemoveFrom.empty()) {
        removePipeline(renderStage, subPass, pipeline);
    }
}

auto trc::SceneBase::insertRegistration(DrawableExecutionRegistration entry) -> RegistrationID
{
    assert(RenderStage::at(entry.renderStage).getNumSubPasses() > entry.subPass);

    const auto renderStage = entry.renderStage;
    const auto subPass = entry.subPass;
    const auto pipeline = entry.pipeline;
    auto& currentRegistrationArray = drawableRegistrations[renderStage][subPass][pipeline];

    // Insert before end to get the iterator. I hope this makes it kind of thread safe
    auto it = currentRegistrationArray.emplace(currentRegistrationArray.end(), std::move(entry));

    // Make long-living index in the pipeline's registration array
    ui32 newIndex = static_cast<ui32>(it - currentRegistrationArray.begin());
    it->thisPointer = std::make_unique<DrawableExecutionRegistration*>(&*it);
    it->indexInRegistrationArray = newIndex;

    return { it->thisPointer.get() };
}

void trc::SceneBase::tryInsertPipeline(
    RenderStage::ID renderStage,
    SubPass::ID subPass,
    GraphicsPipeline::ID pipeline)
{
    auto [it, success] = uniquePipelines[renderStage][subPass].insert(pipeline);
    if (success) {
        uniquePipelinesVector[renderStage][subPass].push_back(pipeline);
    }
}

void trc::SceneBase::removePipeline(
    RenderPass::ID renderStage,
    SubPass::ID subPass,
    GraphicsPipeline::ID pipeline)
{
    uniquePipelines[renderStage][subPass].erase(pipeline);
    uniquePipelinesVector[renderStage][subPass].erase(std::ranges::find(
        uniquePipelinesVector[renderStage][subPass], pipeline
    ));
}
