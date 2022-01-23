#pragma once

#include <vector>
#include <mutex>

#include <componentlib/ComponentStorage.h>
#include <trc/Scene.h>
#include <trc/Camera.h>
#include <trc/drawable/Drawable.h>
using namespace trc::basic_types;

#include "SceneObject.h"

class Scene : public componentlib::ComponentStorage<Scene, SceneObject>
{
public:
    Scene();
    ~Scene();

    void update();

    void saveToFile();
    void loadFromFile();

    auto getCamera() -> trc::Camera&;
    auto getCamera() const -> const trc::Camera&;
    auto getDrawableScene() -> trc::Scene&;

    auto createDefaultObject(trc::Drawable drawable) -> SceneObject;

    /**
     * @brief Add a drawable to the underlying trc::Scene
     */
    template<typename T> requires requires (T a, trc::Scene s) { a.attachToScene(s); }
    void addDrawable(T& drawable)
    {
        drawable.attachToScene(scene);
    }

private:
    trc::Camera camera;
    trc::Scene scene;
    trc::Light sunLight;
};

template<>
struct componentlib::TableTraits<trc::Drawable>
{
    using UniqueStorage = std::true_type;
};

template<>
struct componentlib::ComponentTraits<trc::Drawable>
{
    void onCreate(Scene& scene, SceneObject, trc::Drawable& d)
    {
        scene.addDrawable(d);
    }
};
