#pragma once

#include <memory>

#include <trc/Torch.h>
using namespace trc::basic_types;

#include "Project.h"
#include "Scene.h"
#include "gui/MainMenu.h"
#include "input/InputState.h"
#include "object/Hitbox.h"

class App
{
public:
    explicit App(Project project);
    ~App();

    void run();
    void end();

    auto getProject() -> Project&;

    auto getTorch() -> trc::TorchStack&;
    auto getAssets() -> trc::AssetManager&;
    auto getScene() -> Scene&;

    static auto get() -> App&;

private:
    static inline App* _app{ nullptr };

    /** I try to limit the initialization hacks to only this single one */
    bool initGlobalState;

    void init();
    void tick();
    bool doEnd{ false };

    Project project;

    u_ptr<int, void(*)(int*)> torchTerminator;
    u_ptr<trc::TorchStack> torch;
    u_ptr<trc::imgui::ImguiRenderPass> imgui{ nullptr };
    trc::AssetManager* assetManager;
    Scene scene;

    gui::MainMenu mainMenu;
    InputStateMachine inputState;

    trc::Timer frameTimer;
};
