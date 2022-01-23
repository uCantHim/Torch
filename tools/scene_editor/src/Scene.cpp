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
    ).setProjectionMatrix(glm::ortho(-15.0f, 15.0f, -15.0f, 15.0f, -50.0f, 50.0f));;
}

Scene::~Scene()
{
    ComponentStorage::clear();
}

void Scene::update()
{
    scene.updateTransforms();

    const vec3 mousePos = App::getTorch().getRenderConfig().getMouseWorldPos(camera);
    float closestDist{ std::numeric_limits<float>::max() };
    SceneObject closestObject{ SceneObject::NONE };
    for (const auto& [key, hitbox] : get<Hitbox>().items())
    {
        if (hitbox.isInside(mousePos))
        {
            std::cout << "Mouse is inside of " << key << "\n";
            const float dist = distance(mousePos, hitbox.getSphere().position);
            if (dist < closestDist)
            {
                closestDist = dist;
                closestObject = key;
            }
        }
    }

    std::cout << "Closest object is " << closestObject << "\n";
}

auto Scene::getCamera() -> trc::Camera&
{
    return camera;
}

auto Scene::getCamera() const -> const trc::Camera&
{
    return camera;
}

auto Scene::getDrawableScene() -> trc::Scene&
{
    return scene;
}

auto Scene::createDefaultObject(trc::Drawable drawable) -> SceneObject
{
    auto obj = createObject();
    auto& node = add<ObjectBaseNode>(obj);
    auto& d = add<trc::Drawable>(obj, std::move(drawable));
    node.attach(d);

    add<Hitbox>(obj, App::getAssets().getHitbox(d.getGeometry()));

    return obj;
}
