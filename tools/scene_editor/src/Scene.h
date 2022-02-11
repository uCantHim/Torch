#pragma once

#include <vector>
#include <mutex>

#include <componentlib/ComponentStorage.h>
#include <trc/Torch.h>
using namespace trc::basic_types;

#include "object/SceneObject.h"
#include "object/ObjectSelection.h"

class App;
class AssetManager;

class Scene : public componentlib::ComponentStorage<Scene, SceneObject>
{
public:
    explicit Scene(App& app);
    ~Scene();

    void update();

    void saveToFile();
    void loadFromFile();

    auto getTorch() -> trc::TorchStack&;
    auto getAssets() -> AssetManager&;
    auto getCamera() -> trc::Camera&;
    auto getCamera() const -> const trc::Camera&;
    auto getDrawableScene() -> trc::Scene&;

    void openContextMenu();
    void selectHoveredObject();
    auto getHoveredObject() -> trc::Maybe<SceneObject>;
    auto getSelectedObject() -> trc::Maybe<SceneObject>;

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
    void calcObjectHover();

    App* app;

    trc::Camera camera;
    trc::Scene scene;
    trc::Light sunLight;

    ObjectSelection objectSelection;
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
