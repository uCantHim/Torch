#include "base/SceneRegisterable.h"



void trc::SceneRegisterable::usePipeline(
    RenderStage::ID renderStage,
    SubPass::ID subPass,
    Pipeline::ID pipeline,
    DrawableFunction recordCommandBufferFunction)
{
    auto& func = drawableRecordFuncs.emplace_back(
        renderStage, subPass, pipeline, std::move(recordCommandBufferFunction)
    );

    if (currentScene != nullptr)
    {
        registrationIDs.push_back(
            currentScene->registerDrawFunction(renderStage, subPass, pipeline, std::get<3>(func))
        );
    }
}

void trc::SceneRegisterable::attachToScene(SceneBase& scene)
{
    if (currentScene != nullptr)
    {
        removeFromScene();
    }

    for (const auto& [renderStage, subPass, pipeline, func] : drawableRecordFuncs)
    {
        registrationIDs.push_back(scene.registerDrawFunction(renderStage, subPass, pipeline, func));
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
