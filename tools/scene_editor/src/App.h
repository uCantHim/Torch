#pragma once

#include <trc/Torch.h>
#include <trc_util/Timer.h>
using namespace trc::basic_types;

#include "Scene.h"
#include "asset/AssetInventory.h"
#include "graphics/Graphics.h"
#include "graphics/Window.h"
#include "input/InputProcessor.h"
#include "viewport/SceneViewport.h"
#include "viewport/ViewportTree.h"
#include "viewport/ViewportTreeController.h"

struct KeyConfig;

class App
{
public:
    explicit App(const fs::path& projectRootDir);
    ~App();

    void run();
    void end();

    auto getMainWindow() -> trc::Window&;
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

    /**
     * @brief Set up key bindings for the root frame.
     */
    void setupRootInputFrame(InputFrame& frame, const KeyConfig& conf);

    GraphicsStack graphics;
    s_ptr<trc::Window> mainTorchWindow;

    s_ptr<trc::DataStorage> assetDataStorage;
    trc::AssetManager assetManager;
    AssetInventory assetInventory;

    s_ptr<trc::Camera> camera;
    s_ptr<trc::Scene> drawableScene;
    s_ptr<Scene> scene;

    s_ptr<trc::RenderPipeline> sceneRenderPipeline;
    s_ptr<SceneViewport> sceneViewport;

    s_ptr<Window> mainWindow;

    trc::Timer frameTimer;
};
