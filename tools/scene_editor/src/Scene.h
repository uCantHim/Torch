#pragma once

#include <componentlib/ComponentStorage.h>
#include <trc/Torch.h>
using namespace trc::basic_types;

#include "input/UserInput.h"
#include "object/ObjectSelection.h"
#include "object/SceneObject.h"
#include "scene/CameraArm.h"

class App;
struct Ray;

class Scene : public componentlib::ComponentStorage<Scene, SceneObject>
{
public:
    Scene(App& app, s_ptr<trc::Camera> camera, s_ptr<trc::Scene> scene);
    ~Scene();

    void update(float timeDelta);
    void notifyCursorMove(const CursorMovement& cursor);

    void saveToFile();
    void loadFromFile();

    auto getCamera() -> trc::Camera&;
    auto getCamera() const -> const trc::Camera&;
    auto getCameraArm() -> CameraArm&;
    auto getDrawableScene() -> trc::Scene&;

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
    void calcObjectHover(vec2 cursorPos, uvec2 viewportSize);

    App* app;

    s_ptr<trc::Camera> camera;
    CameraArm cameraArm;

    s_ptr<trc::Scene> scene;
    trc::SunLight sunLight;

    ObjectSelection objectSelection;
};
