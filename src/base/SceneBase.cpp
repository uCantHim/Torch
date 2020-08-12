#include "base/SceneBase.h"



trc::SceneBase::DrawableExecutionRegistration::DrawableExecutionRegistration(
    RenderPass::ID r,
    SubPass::ID s,
    GraphicsPipeline::ID p,
    DrawableFunction func)
    :
    renderPass(r), subPass(s), pipeline(p), recordFunction(func)
{}



auto trc::SceneBase::getPipelines(RenderPass::ID renderPass, SubPass::ID subpass) const noexcept
    -> const std::set<GraphicsPipeline::ID>&
{
    static std::set<GraphicsPipeline::ID> emptyResult;

    if (uniquePipelines.size() <= renderPass
        || uniquePipelines[renderPass].size() <= subpass)
    {
        return emptyResult;
    }

    return uniquePipelines[renderPass][subpass];
}

void trc::SceneBase::invokeDrawFunctions(
    RenderPass::ID renderPass,
    SubPass::ID subpass,
    GraphicsPipeline::ID pipeline,
    vk::CommandBuffer cmdBuf) const
{
    DrawEnvironment env{
        .currentRenderPass = &RenderPass::at(renderPass),
        .currentSubPass = subpass,
        .currentPipeline = &GraphicsPipeline::at(pipeline)
    };

    for (auto& f : drawableRegistrations[renderPass][subpass][pipeline])
    {
        f.recordFunction(env, cmdBuf);
    }
}

auto trc::SceneBase::registerDrawFunction(
    RenderPass::ID renderPass,
    SubPass::ID subpass,
    GraphicsPipeline::ID usedPipeline,
    DrawableFunction commandBufferRecordingFunction
    ) -> RegistrationID
{
    tryInsertPipeline(renderPass, subpass, usedPipeline);

    return insertRegistration(
        { renderPass, subpass, usedPipeline, std::move(commandBufferRecordingFunction) }
    );
}

void trc::SceneBase::unregisterDrawFunction(RegistrationID id)
{
    DrawableExecutionRegistration* entry = *id.reg;

    const auto renderPass = entry->renderPass;
    const auto subpass = entry->subPass;
    const auto pipeline = entry->pipeline;
    const auto index = entry->indexInRegistrationArray;

    auto& vectorToRemoveFrom = drawableRegistrations[renderPass][subpass][pipeline];

    auto& movedElem = vectorToRemoveFrom.back();
    std::swap(vectorToRemoveFrom[index], movedElem);

    // Set backreferencing values
    *movedElem.thisPointer = &movedElem;
    movedElem.indexInRegistrationArray = index;

    vectorToRemoveFrom.pop_back();
    if (vectorToRemoveFrom.empty()) {
        removePipeline(renderPass, subpass, pipeline);
    }
}

auto trc::SceneBase::insertRegistration(DrawableExecutionRegistration entry) -> RegistrationID
{
    assert(RenderPass::at(entry.renderPass).getNumSubPasses() > entry.subPass);

    const auto renderPass = entry.renderPass;
    const auto subPass = entry.subPass;
    const auto pipeline = entry.pipeline;
    auto& currentRegistrationArray = drawableRegistrations[renderPass][subPass][pipeline];

    // Insert before end to get the iterator. I hope this makes it kind of thread safe
    auto it = currentRegistrationArray.emplace(currentRegistrationArray.end(), std::move(entry));

    // Make long-living index in the pipeline's registration array
    ui32 newIndex = static_cast<ui32>(it - currentRegistrationArray.begin());
    it->thisPointer = std::make_unique<DrawableExecutionRegistration*>(&*it);
    it->indexInRegistrationArray = newIndex;

    return { it->thisPointer.get() };
}

void trc::SceneBase::tryInsertPipeline(
    RenderPass::ID renderPass,
    SubPass::ID subpass,
    GraphicsPipeline::ID pipeline)
{
    auto [it, success] = uniquePipelines[renderPass][subpass].insert(pipeline);
    if (success) {
        uniquePipelinesVector[renderPass][subpass].push_back(pipeline);
    }
}

void trc::SceneBase::removePipeline(
    RenderPass::ID renderPass,
    SubPass::ID subpass,
    GraphicsPipeline::ID pipeline)
{
    uniquePipelines[renderPass][subpass].erase(pipeline);
    uniquePipelinesVector[renderPass][subpass].erase(std::find(
        uniquePipelinesVector[renderPass][subpass].begin(),
        uniquePipelinesVector[renderPass][subpass].end(),
        pipeline
    ));
}
