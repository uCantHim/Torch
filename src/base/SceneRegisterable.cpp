#include "base/SceneRegisterable.h"



void trc::SceneRegisterable::usePipeline(
    SubPass::ID subPass,
    GraphicsPipeline::ID pipeline,
    DrawableFunction recordCommandBufferFunction)
{
    drawableRecordFuncs.emplace_back(subPass, pipeline, std::move(recordCommandBufferFunction));
}

void trc::SceneRegisterable::attachToScene(SceneBase& scene)
{
    if (!registrationIDs.empty())
    {
        removeFromScene();
    }

    for (const auto& [subPass, pipeline, func] : drawableRecordFuncs)
    {
        registrationIDs.push_back(scene.registerDrawFunction(subPass, pipeline, func));
    }
    currentScene = &scene;
}

void trc::SceneRegisterable::removeFromScene()
{
    assert(currentScene != nullptr);

    for (auto id : registrationIDs)
    {
        currentScene->unregisterDrawFunction(id);
    }
    registrationIDs.clear();
}
