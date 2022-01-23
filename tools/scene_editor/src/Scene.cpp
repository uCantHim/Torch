#include "Scene.h"

#include <vkb/event/WindowEvents.h>

#include "App.h"



Scene::Scene(App& app)
    :
    app(&app)
{
    auto recalcProjMat = [this](const vkb::SwapchainResizeEvent& e)
    {
        auto size = e.swapchain->getImageExtent();
        camera.makePerspective(float(size.width) / float(size.height), 45.0f, 0.5f, 50.0f);
    };
    recalcProjMat({ { &app.getTorch().getWindow() } });
    camera.lookAt({ 5, 5, 5 }, { 0, 0, 0 }, { 0, 1, 0 });
    vkb::on<vkb::SwapchainResizeEvent>(recalcProjMat);

    // Enable shadows for the sun
    sunLight = scene.getLights().makeSunLight(vec3(1, 1, 1), vec3(1, -1, -1), 0.6f);
    scene.enableShadow(
        sunLight,
        trc::ShadowCreateInfo{ .shadowMapResolution={ 4096, 4096 } },
        app.getTorch().getShadowPool()
    ).setProjectionMatrix(glm::ortho(-15.0f, 15.0f, -15.0f, 15.0f, -50.0f, 50.0f));;
}

Scene::~Scene()
{
    ComponentStorage::clear();
}

void Scene::update()
{
    scene.updateTransforms();

    calcObjectHover();
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

    scene.getRoot().attach(node);

    add<Hitbox>(obj, app->getAssets().getHitbox(d.getGeometry()));

    return obj;
}

void Scene::calcObjectHover()
{
    const vec3 mousePos = app->getTorch().getRenderConfig().getMouseWorldPos(camera);

    float closestDist{ std::numeric_limits<float>::max() };
    SceneObject closestObject{ SceneObject::NONE };
    for (const auto& [key, hitbox, node] : get<Hitbox>().join(get<ObjectBaseNode>()))
    {
        mat4 toObjectSpace = glm::inverse(node.getGlobalTransform());
        if (hitbox.isInside(toObjectSpace * vec4(mousePos, 1.0f)))
        {
            const float dist = distance(mousePos, hitbox.getSphere().position);
            if (dist < closestDist)
            {
                closestDist = dist;
                closestObject = key;
            }
        }
    }

    objectSelection.hoverObject(closestObject);
}
