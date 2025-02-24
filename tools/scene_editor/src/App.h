#pragma once

#include <trc/Torch.h>
#include <trc_util/Timer.h>
using namespace trc::basic_types;

#include "Scene.h"
#include "asset/AssetInventory.h"
#include "viewport/SceneViewport.h"
#include "viewport/ViewportTree.h"

class App
{
public:
    explicit App(const fs::path& projectRootDir);
    ~App();

    void run();
    void end();

    auto getTorch() -> trc::TorchStack&;
    auto getAssets() -> AssetInventory&;
    auto getScene() -> Scene&;

    auto getViewportManager() -> ViewportTree&;
    auto getSceneViewport() -> ViewportArea;

    static auto get() -> App&;

private:
    static constexpr vec3 kClearColor{ 0.12f, 0.12f, 0.12f };

    static inline App* _app{ nullptr };

    /** I try to limit the initialization hacks to only this single one */
    bool initGlobalState;

    void init();
    void tick();
    bool doEnd{ false };

    u_ptr<int, void(*)(int*)> torchTerminator;
    u_ptr<trc::TorchStack> torch;
    trc::Renderer renderer;

    AssetInventory assetInventory;

    s_ptr<trc::Camera> camera;
    s_ptr<trc::Scene> drawableScene;
    s_ptr<Scene> scene;

    s_ptr<SceneViewport> sceneViewport;
    s_ptr<ViewportTree> viewportManager;

    trc::Timer frameTimer;
};
