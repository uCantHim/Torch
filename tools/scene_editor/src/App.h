#pragma once

#include <trc/Torch.h>
#include <trc_util/Timer.h>
using namespace trc::basic_types;

#include "Scene.h"
#include "gui/MainMenu.h"
#include "input/EventTarget.h"
#include "viewport/SceneViewport.h"

/**
 * An ad-hoc proof of concept thing.
 */
struct ViewportManager : public EventTarget
{
    ViewportManager(App& app, u_ptr<SceneViewport> sceneVp);

    void resize(uvec2 size);
    void setSceneViewportPos(float horizontalPos);

    void notify(const UserInput& input) override;
    void notify(const Scroll& scroll) override;
    void notify(const CursorMovement& cursorMove) override;

    gui::MainMenu mainMenuViewport;
    u_ptr<SceneViewport> sceneViewport;

    float horizontalSceneVpPos{ 0.25f };
};

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

    auto getSceneViewport() -> ViewportArea;

    static auto get() -> App&;

private:
    static inline App* _app{ nullptr };

    /** I try to limit the initialization hacks to only this single one */
    bool initGlobalState;

    void init();
    void tick();
    bool doEnd{ false };

    u_ptr<int, void(*)(int*)> torchTerminator;
    u_ptr<trc::TorchStack> torch;

    AssetInventory assetInventory;

    s_ptr<trc::Camera> camera;
    s_ptr<trc::Scene> drawableScene;
    s_ptr<Scene> scene;

    trc::ViewportHandle sceneVp;
    s_ptr<ViewportManager> viewportManager;

    trc::Timer frameTimer;
};
