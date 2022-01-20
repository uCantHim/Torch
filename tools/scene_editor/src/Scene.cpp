#include "Scene.h"

#include <vkb/event/WindowEvents.h>

#include "App.h"



Scene::Scene()
    :
    sunLight()
{
    auto recalcProjMat = [this](const vkb::SwapchainResizeEvent& e)
    {
        auto size = e.swapchain->getImageExtent();
        camera.makePerspective(float(size.width) / float(size.height), 45.0f, 0.5f, 50.0f);
    };
    recalcProjMat({ { &App::getTorch().getWindow() } });
    camera.lookAt({ 5, 5, 5 }, { 0, 0, 0 }, { 0, 1, 0 });
    vkb::on<vkb::SwapchainResizeEvent>(recalcProjMat);

    // Enable shadows for the sun
    sunLight = scene.getLights().makeSunLight(vec3(1, 1, 1), vec3(1, -1, -1), 0.6f);
    scene.enableShadow(
        sunLight,
        trc::ShadowCreateInfo{ .shadowMapResolution={ 4096, 4096 } },
        App::getTorch().getShadowPool()
    ).setProjectionMatrix(glm::ortho(-15.0f, 15.0f, -15.0f, 15.0f, -30.0f, 30.0f));;
}

void Scene::update()
{
    scene.updateTransforms();
}

auto Scene::getCamera() -> trc::Camera&
{
    return camera;
}

auto Scene::getDrawableScene() -> trc::Scene&
{
    return scene;
}

auto Scene::addObject(std::unique_ptr<trc::Drawable> drawable) -> SceneObject::ID
{
    // Construct new object
    const SceneObject::ID newID{ getNextIndex() };
    objects.at(newID).reset(new SceneObject(newID, std::move(drawable)));
    auto& newObj = *objects.at(newID);

    // Attach object to scene
    newObj.getDrawable().attachToScene(scene);
    scene.getRoot().attach(newObj.getSceneNode());

    return newID;
}

auto Scene::getObject(SceneObject::ID id) -> SceneObject&
{
    auto& objPtr = objects.at(id);
    if (objPtr == nullptr) {
        throw std::out_of_range("Object at index " + std::to_string(id) + " does not exist");
    }

    return *objPtr;
}

void Scene::removeObject(SceneObject::ID id)
{
    if (objects.at(id) != nullptr) {
        objectIdPool.free(id);
    }

    objects.at(id).reset();
}

auto Scene::getNextIndex() -> ui32
{
    ui32 id = objectIdPool.generate();

    std::lock_guard lock(objectListLock);
    if (id >= objects.size()) {
        objects.emplace_back();
    }

    return id;
}
