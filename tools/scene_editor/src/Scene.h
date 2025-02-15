#pragma once

#include <componentlib/ComponentStorage.h>
#include <trc/Torch.h>
using namespace trc::basic_types;

#include "object/ObjectSelection.h"
#include "object/SceneObject.h"

class App;
struct Ray;

class Scene : public componentlib::ComponentStorage<Scene, SceneObject>
{
public:
    Scene(App& app, s_ptr<trc::Camera> camera, s_ptr<trc::Scene> scene);
    ~Scene();

    void update(float timeDelta);

    void saveToFile();
    void loadFromFile();

    auto getTorch() -> trc::TorchStack&;
    auto getCamera() -> trc::Camera&;
    auto getCamera() const -> const trc::Camera&;
    auto getCameraViewNode() -> trc::Node&;
    auto getDrawableScene() -> trc::Scene&;

    auto getMouseDepth() const -> float;
    auto getMousePosAtDepth(float depth) const -> vec3;
    auto getMouseWorldPos() const -> vec3;

    /**
     * @brief Create an object
     *
     * Overrides ComponentStorage::createObject for some wrapper
     * functionality.
     *
     * @return SceneObject An object with no components and no functionality.
     */
    auto createObject() -> SceneObject;

    /**
     * @brief Delete an object and all of its components
     *
     * Overrides ComponentStorage::deleteObject for some wrapper
     * functionality.
     */
    void deleteObject(SceneObject obj);

    auto iterObjects() const
        -> trc::algorithm::IteratorRange<componentlib::Table<ObjectMetadata, SceneObject>::const_iterator>;

    /**
     * @brief Open a context menu for the currenty hovered object.
     *
     * Does nothing if no object is being hovered.
     */
    void openContextMenu();
    void selectHoveredObject();
    auto getHoveredObject() -> trc::Maybe<SceneObject>;
    auto getSelectedObject() -> trc::Maybe<SceneObject>;

    void selectObject(SceneObject obj);
    void hoverObject(SceneObject obj);

    auto createDefaultObject(trc::Drawable drawable) -> SceneObject;
    auto createDefaultObject(const trc::DrawableCreateInfo& createInfo) -> SceneObject;

    /**
     * @brief Cast a ray into the scene.
     *
     * @return The first object hit by the ray, and the hit position.
     */
    auto castRay(const Ray& ray) -> std::optional<std::pair<SceneObject, vec3>>;

private:
    void calcObjectHover();

    App* app;

    s_ptr<trc::Camera> camera;
    s_ptr<trc::Scene> scene;
    trc::Node cameraViewNode;
    trc::SunLight sunLight;

    ObjectSelection objectSelection;
};
