#include "Scene.h"



Scene::DrawableExecutionRegistration::DrawableExecutionRegistration(
    SubPass::ID s,
    GraphicsPipeline::ID p,
    std::function<void(vk::CommandBuffer)> func)
    :
    subPass(s), pipeline(p), recordFunction(func)
{}



auto Scene::getPipelines(SubPass::ID subpass) const noexcept
    -> const std::vector<GraphicsPipeline::ID>&
{
    return uniquePipelinesVector[subpass];
}

void Scene::invokeDrawFunctions(
    SubPass::ID subpass,
    GraphicsPipeline::ID pipeline,
    vk::CommandBuffer cmdBuf) const
{
    for (auto& f : drawableRegistrations[subpass][pipeline])
    {
        f.recordFunction(cmdBuf);
    }
}

auto Scene::registerDrawFunction(
    SubPass::ID subpass,
    GraphicsPipeline::ID usedPipeline,
    std::function<void(vk::CommandBuffer)> commandBufferRecordingFunction
    ) -> RegistrationID
{
    std::cout << "Registering draw function at subpass " << subpass
        << " and pipeline " << usedPipeline << "\n";

    tryInsertPipeline(subpass, usedPipeline);

    return insertRegistration(
        { subpass, usedPipeline, std::move(commandBufferRecordingFunction) }
    );
}

void Scene::unregisterDrawFunction(RegistrationID id)
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

auto Scene::insertRegistration(DrawableExecutionRegistration entry) -> RegistrationID
{
    auto& currentRegistrationArray = drawableRegistrations[entry.subPass][entry.pipeline];

    // Insert before end to get the iterator. I hope this makes it kind of thread safe
    auto it = currentRegistrationArray.emplace(currentRegistrationArray.end(), std::move(entry));

    // Make long-living index in the pipeline's registration array
    uint32_t newIndex = static_cast<uint32_t>(it - currentRegistrationArray.begin());
    it->thisPointer = std::make_unique<DrawableExecutionRegistration*>(&*it);
    it->indexInRegistrationArray = newIndex;

    return { it->thisPointer.get() };
}

void Scene::tryInsertPipeline(SubPass::ID subpass, GraphicsPipeline::ID pipeline)
{
    auto [it, success] = uniquePipelines[subpass].insert(pipeline);
    if (success) {
        uniquePipelinesVector[subpass].push_back(pipeline);
    }
}

void Scene::removePipeline(SubPass::ID subpass, GraphicsPipeline::ID pipeline)
{
    uniquePipelines[subpass].erase(pipeline);
    std::remove(
        uniquePipelinesVector[subpass].begin(),
        uniquePipelinesVector[subpass].end(),
        pipeline
    );
}
