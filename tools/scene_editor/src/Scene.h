#pragma once

#include <vector>
#include <mutex>

#include <trc/Scene.h>

#include "SceneObject.h"
#include "input/CameraController.h"

class Scene
{
public:
    Scene();

    void update();

    void saveToFile();
    void loadFromFile();

    auto getCamera() -> trc::Camera&;
    auto getDrawableScene() -> trc::Scene&;

    /**
     * @brief Add a drawable to the underlying trc::Scene
     */
    template<typename T>
    void addDrawable(T& drawable)
        requires requires (T a, trc::Scene s) { a.attachToScene(s); }
    {
        drawable.attachToScene(scene);
    }

    auto addObject(std::unique_ptr<trc::Drawable> drawable) -> SceneObject::ID;
    auto getObject(SceneObject::ID id) -> SceneObject&;
    void removeObject(SceneObject::ID id);

private:
    trc::Camera camera;
    trc::Scene scene;
    trc::Light sunLight;

    input::CameraController cameraController{ camera };

    auto getNextIndex() -> ui32;
    std::mutex objectListLock;
    std::vector<std::unique_ptr<SceneObject>> objects;
    trc::data::IdPool objectIdPool;
};
