#include "Scene.h"

#include <trc/base/event/WindowEvents.h>

#include "App.h"
#include "gui/ContextMenu.h"
#include "object/Context.h"
#include "object/Hitbox.h"



Scene::Scene(App& app)
    :
    app(&app),
    objectSelection(*this)
{
    auto recalcProjMat = [this](const trc::SwapchainResizeEvent& e)
    {
        auto size = e.swapchain->getImageExtent();
        camera.makePerspective(float(size.width) / float(size.height), 45.0f, 0.5f, 200.0f);
    };
    recalcProjMat({ { &app.getTorch().getWindow() } });

    scene.getRoot().attach(cameraViewNode);
    cameraViewNode.attach(camera);
    cameraViewNode.setFromMatrix(glm::lookAt(vec3(5, 5, 5), vec3(0, 0, 0), vec3(0, 1, 0)));
    trc::on<trc::SwapchainResizeEvent>(recalcProjMat);

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

auto Scene::getAssets() -> trc::AssetManager&
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

auto Scene::createObject() -> SceneObject
{
    auto obj = ComponentStorage::createObject();
    add<ObjectMetadata>(obj, ObjectMetadata{ .id=obj, .name="_unnamed_" });

    return obj;
}

void Scene::deleteObject(SceneObject obj)
{
    ComponentStorage::deleteObject(obj);
}

auto Scene::iterObjects() const
    -> trc::algorithm::IteratorRange<componentlib::Table<ObjectMetadata, SceneObject>::const_iterator>
{
    using const_iterator = componentlib::Table<ObjectMetadata, SceneObject>::const_iterator;

    const auto& meta = get<ObjectMetadata>();
    return trc::algorithm::IteratorRange<const_iterator>{ meta.begin(), meta.end() };
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
    getHoveredObject().maybe(
        [&](SceneObject obj) { objectSelection.selectObject(obj); },
        [&]() { objectSelection.unselectObject(); }
    );
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

void Scene::selectObject(SceneObject obj)
{
    if (obj == SceneObject::NONE) {
        objectSelection.unselectObject();
    }
    else {
        objectSelection.selectObject(obj);
    }
}

void Scene::hoverObject(SceneObject obj)
{
    if (obj == SceneObject::NONE) {
        objectSelection.unhoverObject();
    }
    else {
        objectSelection.hoverObject(obj);
    }
}

auto Scene::createDefaultObject(trc::Drawable drawable) -> SceneObject
{
    auto obj = createObject();
    auto& node = add<ObjectBaseNode>(obj);
    auto& d = add<trc::Drawable>(obj, std::move(drawable));
    node.attach(d);
    scene.getRoot().attach(node);

    // Create hitbox component
    auto& hitboxes = app->getAssets().getModule<HitboxAsset>();
    add<Hitbox>(obj, hitboxes.getForGeometry(d.getGeometry()));

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
