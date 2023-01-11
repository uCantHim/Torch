#pragma once

#include <vector>
#include <mutex>

#include <componentlib/ComponentStorage.h>
#include <trc/Torch.h>
using namespace trc::basic_types;

#include "object/SceneObject.h"
#include "object/ObjectSelection.h"

class App;

class Scene : public componentlib::ComponentStorage<Scene, SceneObject>
{
public:
    explicit Scene(App& app);
    ~Scene();

    void update(float timeDelta);

    void saveToFile();
    void loadFromFile();

    auto getTorch() -> trc::TorchStack&;
    auto getAssets() -> trc::AssetManager&;
    auto getCamera() -> trc::Camera&;
    auto getCamera() const -> const trc::Camera&;
    auto getDrawableScene() -> trc::Scene&;

    auto getMouseDepth() const -> float;
    auto getMousePosAtDepth(float depth) const -> vec3;
    auto getMouseWorldPos() const -> vec3;

    void openContextMenu();
    void selectHoveredObject();
    auto getHoveredObject() -> trc::Maybe<SceneObject>;
    auto getSelectedObject() -> trc::Maybe<SceneObject>;

    auto createDefaultObject(trc::Drawable drawable) -> SceneObject;
    auto createDefaultObject(trc::DrawableCreateInfo createInfo) -> SceneObject;

private:
    void calcObjectHover();

    App* app;

    trc::Node cameraViewNode;
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
