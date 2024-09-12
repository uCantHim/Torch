#pragma once

#include <trc/Torch.h>
#include <trc_util/Timer.h>
using namespace trc::basic_types;

#include "Scene.h"
#include "gui/MainMenu.h"
#include "input/InputState.h"

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

    void setSceneViewport(vec2 offset, vec2 size);

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

    s_ptr<trc::Camera> camera;
    s_ptr<trc::Scene> drawableScene;
    s_ptr<Scene> scene;
    trc::ViewportHandle mainViewport;

    AssetInventory assetInventory;

    gui::MainMenu mainMenu;
    InputStateMachine inputState;

    trc::Timer frameTimer;
};
