#include "base/SceneBase.h"



trc::SceneBase::DrawableExecutionRegistration::DrawableExecutionRegistration(
    SubPass::ID s,
    GraphicsPipeline::ID p,
    DrawableFunction func)
    :
    subPass(s), pipeline(p), recordFunction(func)
{}



auto trc::SceneBase::getPipelines(SubPass::ID subpass) const noexcept
    -> const std::vector<GraphicsPipeline::ID>&
{
    return uniquePipelinesVector[subpass];
}

void trc::SceneBase::invokeDrawFunctions(
    SubPass::ID subpass,
    GraphicsPipeline::ID pipeline,
    vk::CommandBuffer cmdBuf) const
{
    for (auto& f : drawableRegistrations[subpass][pipeline])
    {
        f.recordFunction(cmdBuf);
    }
}

auto trc::SceneBase::registerDrawFunction(
    SubPass::ID subpass,
    GraphicsPipeline::ID usedPipeline,
    DrawableFunction commandBufferRecordingFunction
    ) -> RegistrationID
{
    tryInsertPipeline(subpass, usedPipeline);

    return insertRegistration(
        { subpass, usedPipeline, std::move(commandBufferRecordingFunction) }
    );
}

void trc::SceneBase::unregisterDrawFunction(RegistrationID id)
{
    DrawableExecutionRegistration* entry = *id.reg;

    auto subpass = entry->subPass;
    auto pipeline = entry->pipeline;
    auto index = entry->indexInRegistrationArray;

    auto& vectorToRemoveFrom = drawableRegistrations[subpass][pipeline];

    auto& movedElem = vectorToRemoveFrom.back();
    std::swap(vectorToRemoveFrom[index], movedElem);

    // Set backreferencing values
    *movedElem.thisPointer = &movedElem;
    movedElem.indexInRegistrationArray = index;

    vectorToRemoveFrom.pop_back();
    if (vectorToRemoveFrom.empty()) {
        removePipeline(subpass, pipeline);
    }
}

auto trc::SceneBase::insertRegistration(DrawableExecutionRegistration entry) -> RegistrationID
{
    auto& currentRegistrationArray = drawableRegistrations[entry.subPass][entry.pipeline];

    // Insert before end to get the iterator. I hope this makes it kind of thread safe
    auto it = currentRegistrationArray.emplace(currentRegistrationArray.end(), std::move(entry));

    // Make long-living index in the pipeline's registration array
    ui32 newIndex = static_cast<ui32>(it - currentRegistrationArray.begin());
    it->thisPointer = std::make_unique<DrawableExecutionRegistration*>(&*it);
    it->indexInRegistrationArray = newIndex;

    return { it->thisPointer.get() };
}

void trc::SceneBase::tryInsertPipeline(SubPass::ID subpass, GraphicsPipeline::ID pipeline)
{
    auto [it, success] = uniquePipelines[subpass].insert(pipeline);
    if (success) {
        uniquePipelinesVector[subpass].push_back(pipeline);
    }
}

void trc::SceneBase::removePipeline(SubPass::ID subpass, GraphicsPipeline::ID pipeline)
{
    uniquePipelines[subpass].erase(pipeline);
    uniquePipelinesVector[subpass].erase(std::find(
        uniquePipelinesVector[subpass].begin(),
        uniquePipelinesVector[subpass].end(),
        pipeline
    ));
}
