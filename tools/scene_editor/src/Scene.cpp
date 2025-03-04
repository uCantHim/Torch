#include "Scene.h"

#include "App.h"
#include "gui/ContextMenu.h"
#include "object/Context.h"
#include "object/Hitbox.h"



Scene::Scene(App& app, s_ptr<trc::Camera> _camera, s_ptr<trc::Scene> _scene)
    :
    app(&app),
    camera(_camera),
    cameraArm(_camera, vec3(0, 5, -5), vec3(0, 0, 0), vec3(0, 1, 0)),
    scene(_scene),
    objectSelection(*this)
{
    auto recalcProjMat = [this](const trc::Swapchain& swapchain)
    {
        auto size = swapchain.getImageExtent();
        camera->makePerspective(float(size.width) / float(size.height), 45.0f, 0.5f, 200.0f);
    };
    recalcProjMat(app.getTorch().getWindow());
    app.getTorch().getWindow().addCallbackOnResize(recalcProjMat);

    // Init camera view.
    scene->getRoot().attach(cameraArm);

    // Create a sun light.
    sunLight = scene->getLights().makeSunLight(vec3(1, 1, 1), vec3(1, -1, -1), 0.6f);

    // Enable shadows for the sun light.
    auto shadow = scene->getLights().enableShadow(sunLight, uvec2{ 4096, 4096 });
    shadow->getCamera().makeOrthogonal(-15.0f, 15.0f, -15.0f, 15.0f, -50.0f, 20.0f);
}

Scene::~Scene()
{
    ComponentStorage::clear();
}

void Scene::update(const float timeDelta)
{
    scene->update(timeDelta);
    calcObjectHover();
}

auto Scene::getTorch() -> trc::TorchStack&
{
    return app->getTorch();
}

auto Scene::getCamera() -> trc::Camera&
{
    return *camera;
}

auto Scene::getCamera() const -> const trc::Camera&
{
    return *camera;
}

auto Scene::getCameraArm() -> CameraArm&
{
    return cameraArm;
}

auto Scene::getDrawableScene() -> trc::Scene&
{
    return *scene;
}

auto Scene::unprojectScreenCoords(vec2 screenPos, float depth) -> vec3
{
    const auto vp = app->getSceneViewport();

    screenPos = { screenPos.x, vp.size.y - screenPos.y };
    const ivec2 vpPos = glm::clamp(ivec2{screenPos} - vp.pos,
                                   vp.pos,
                                   vp.pos + ivec2{vp.size});

    return camera->unproject(vpPos, depth, vp.size);
}

auto Scene::getMousePosAtDepth(const float depth) const -> vec3
{
    const auto vp = app->getSceneViewport();
    const ivec2 mousePosVp = getCursorPosClampedToSceneViewport();

    return camera->unproject(mousePosVp, depth, vp.size);
}

auto Scene::getMouseWorldPos() const -> vec3
{
    return mouseWorldPos;
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
    node.attach(*d);
    scene->getRoot().attach(node);

    // Create hitbox component
    auto& hitboxes = app->getAssets().manager().getModule<HitboxAsset>();
    add<Hitbox>(obj, hitboxes.getForGeometry(d->getGeometry()));

    return obj;
}

auto Scene::createDefaultObject(const trc::DrawableCreateInfo& createInfo) -> SceneObject
{
    return createDefaultObject(getDrawableScene().makeDrawable(createInfo));
}

auto Scene::castRay(const Ray& ray) -> std::optional<std::pair<SceneObject, vec3>>
{
    float closestDist{ std::numeric_limits<float>::max() };
    vec3 hitPos;
    SceneObject closestObject{ SceneObject::NONE };

    for (const auto& [obj, hitbox, node] : get<Hitbox>().join(get<ObjectBaseNode>()))
    {
        // Bring the ray into object space to calculate the intersection.
        const mat4 toObjectSpace = glm::inverse(node.getGlobalTransform());
        const Ray objectSpaceRay{
            toObjectSpace * vec4{ ray.origin, 1.0f },
            glm::normalize(toObjectSpace * vec4{ ray.direction, 0.0f }),
        };

        // Broadphase with spheres
        if (!intersectEdge(objectSpaceRay, hitbox.getSphere())) {
            continue;
        }

        if (auto hit = intersect(objectSpaceRay, hitbox.getBox()))
        {
            // The hit coordinates are in object space, and so is the hit distance;
            // calculate the hit distance in world coordinates.
            const auto hitWorldPos = node.getGlobalTransform() * vec4{ hit->first.hitPoint, 1.0f };
            const float dist = glm::distance(vec3{hitWorldPos}, ray.origin);
            if (dist <= closestDist)
            {
                closestDist = dist;
                hitPos = hitWorldPos;
                closestObject = obj;
            }
        }
    }

    return std::pair{ closestObject, hitPos };
}

auto Scene::getCursorPosInSceneViewport() const -> std::optional<ivec2>
{
    const auto vp = app->getSceneViewport();

    const ivec2 mousePosScreen = app->getTorch().getWindow().getMousePositionLowerLeft();
    const ivec2 mousePosVp = mousePosScreen - vp.pos;
    if (mousePosVp.x < 0
        || mousePosVp.y < 0
        || mousePosVp.x >= static_cast<int>(vp.size.x)
        || mousePosVp.y >= static_cast<int>(vp.size.y))
    {
        // Cursor not in viewport.
        return std::nullopt;
    }

    return mousePosVp;
}

auto Scene::getCursorPosClampedToSceneViewport() const -> ivec2
{
    const auto vp = app->getSceneViewport();

    const ivec2 mousePosScreen = app->getTorch().getWindow().getMousePositionLowerLeft();
    const ivec2 mousePosVp = glm::clamp(mousePosScreen, vp.pos, vp.pos + ivec2{vp.size})
                             - vp.pos;
    return mousePosVp;
}

void Scene::calcObjectHover()
{
    if (!getCursorPosInSceneViewport())
    {
        // Cursor not in viewport.
        return;
    }

    const vec3 mousePos = getMousePosAtDepth(0.5f);
    const vec3 cameraWorldPos = getCameraArm().getCameraWorldPos();

    // A ray from the camera through the cursor into the scene
    const Ray cameraRay{ cameraWorldPos, mousePos - cameraWorldPos };

    if (const auto hit = castRay(cameraRay))
    {
        const auto [closestObject, hitPoint] = *hit;
        objectSelection.hoverObject(closestObject);
        mouseWorldPos = hitPoint;
    }
}
