#include "Scene.h"

#include "App.h"
#include "gui/ContextMenu.h"
#include "object/Context.h"
#include "object/Hitbox.h"



Scene::Scene(App& app, s_ptr<trc::Camera> _camera, s_ptr<trc::Scene> _scene)
    :
    app(&app),
    camera(_camera),
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

    // Init camera view
    scene->getRoot().attach(cameraViewNode);
    cameraViewNode.attach(*camera);
    cameraViewNode.setFromMatrix(glm::lookAt(vec3(0, 5, -5), vec3(0, 0, 0), vec3(0, 1, 0)));

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

auto Scene::getCameraViewNode() -> trc::Node&
{
    return cameraViewNode;
}

auto Scene::getDrawableScene() -> trc::Scene&
{
    return *scene;
}

auto Scene::getMouseDepth() const -> float
{
    return 0.0f; //app->getTorch().getRenderConfig().getMouseDepth();
}

auto Scene::getMousePosAtDepth(const float) const -> vec3
{
    return vec3(std::numeric_limits<float>::max());
    return vec3(0.0f); //app->getTorch().getRenderConfig().getMousePosAtDepth(camera, depth);
}

auto Scene::getMouseWorldPos() const -> vec3
{
    return vec3(std::numeric_limits<float>::max());
    return vec3(0.0f); //app->getTorch().getRenderConfig().getMouseWorldPos(camera);
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

        // Intersect.
        if (auto hit = intersectEdge(objectSpaceRay, hitbox.getSphere()))
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

void Scene::calcObjectHover()
{
    const vec2 mousePosScreen = app->getTorch().getWindow().getMousePositionLowerLeft();
    const auto vp = app->getSceneViewport();

    if (mousePosScreen.x < vp.offset.x
        || mousePosScreen.y < vp.offset.y
        || mousePosScreen.x > vp.offset.x + vp.size.x
        || mousePosScreen.y > vp.offset.y + vp.size.y)
    {
        // Cursor not in viewport.
        return;
    }

    const vec2 mousePosVp = mousePosScreen - vec2{vp.offset};
    const vec4 mousePos = vec4{ camera->unproject(mousePosVp, 0.5f, vp.size), 1.0f };
    const vec4 cameraWorldPos = glm::inverse(camera->getGlobalTransform()) * vec4(0, 0, 0, 1);

    // A ray from the camera throught the cursor into the scene
    const Ray cameraRay{ cameraWorldPos, mousePos - cameraWorldPos };

    if (const auto hit = castRay(cameraRay))
    {
        const auto closestObject = hit->first;
        objectSelection.hoverObject(closestObject);
    }
}
