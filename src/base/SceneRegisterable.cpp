#include "base/SceneRegisterable.h"



void trc::SceneRegisterable::usePipeline(
    RenderPass::ID renderPass,
    SubPass::ID subPass,
    GraphicsPipeline::ID pipeline,
    DrawableFunction recordCommandBufferFunction)
{
    auto& func = drawableRecordFuncs.emplace_back(
        renderPass, subPass, pipeline, std::move(recordCommandBufferFunction)
    );

    if (currentScene != nullptr)
    {
        registrationIDs.push_back(
            currentScene->registerDrawFunction(renderPass, subPass, pipeline, std::get<3>(func))
        );
    }
}

void trc::SceneRegisterable::attachToScene(SceneBase& scene)
{
    if (currentScene != nullptr)
    {
        removeFromScene();
    }

    for (const auto& [renderPass, subPass, pipeline, func] : drawableRecordFuncs)
    {
        registrationIDs.push_back(scene.registerDrawFunction(renderPass, subPass, pipeline, func));
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
    currentScene = nullptr;
}
