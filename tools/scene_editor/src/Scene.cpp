#include "Scene.h"

#include <vkb/event/WindowEvents.h>

#include "App.h"
#include "gui/ContextMenu.h"
#include "object/Context.h"



Scene::Scene(App& app)
    :
    app(&app),
    objectSelection(*this)
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
    ).setProjectionMatrix(glm::ortho(-15.0f, 15.0f, -15.0f, 15.0f, -50.0f, 20.0f));;
}

Scene::~Scene()
{
    ComponentStorage::clear();
}

void Scene::update(const float timeDelta)
{
    scene.update(timeDelta);
    calcObjectHover();
}

auto Scene::getTorch() -> trc::TorchStack&
{
    return app->getTorch();
}

auto Scene::getAssets() -> AssetManager&
{
    return app->getAssets();
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

auto Scene::getMouseDepth() const -> float
{
    return app->getTorch().getRenderConfig().getMouseDepth();
}

auto Scene::getMousePosAtDepth(const float depth) const -> vec3
{
    return app->getTorch().getRenderConfig().getMousePosAtDepth(camera, depth);
}

auto Scene::getMouseWorldPos() const -> vec3
{
    return app->getTorch().getRenderConfig().getMouseWorldPos(camera);
}

void Scene::openContextMenu()
{
    if (objectSelection.hasHoveredObject())
    {
        auto obj = objectSelection.getHoveredObject();
        gui::ContextMenu::show(
            "object " + obj.toString(),
            makeContext(*this, obj)
        );
    }
}

void Scene::selectHoveredObject()
{
    getHoveredObject() >> [&](SceneObject obj) {
        objectSelection.selectObject(obj);
    };
}

auto Scene::getHoveredObject() -> trc::Maybe<SceneObject>
{
    if (objectSelection.hasHoveredObject()) {
        return objectSelection.getHoveredObject();
    }
    return {};
}

auto Scene::getSelectedObject() -> trc::Maybe<SceneObject>
{
    if (objectSelection.hasSelectedObject()) {
        return objectSelection.getSelectedObject();
    }
    return {};
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

auto Scene::createDefaultObject(trc::DrawableCreateInfo createInfo) -> SceneObject
{
    return createDefaultObject(trc::Drawable(createInfo, getDrawableScene()));
}

void Scene::calcObjectHover()
{
    const vec4 mousePos = vec4(app->getTorch().getRenderConfig().getMouseWorldPos(camera), 1.0f);

    float closestDist{ std::numeric_limits<float>::max() };
    SceneObject closestObject{ SceneObject::NONE };
    for (const auto& [key, hitbox, node] : get<Hitbox>().join(get<ObjectBaseNode>()))
    {
        const vec3 objectSpace = glm::inverse(node.getGlobalTransform()) * mousePos;
        if (hitbox.isInside(objectSpace))
        {
            const float dist = distance(objectSpace, hitbox.getSphere().position);
            if (dist <= closestDist)
            {
                closestDist = dist;
                closestObject = key;
            }
        }
    }

    objectSelection.hoverObject(closestObject);
}
