#pragma once

#include <memory>

#include <trc/Torch.h>
using namespace trc::basic_types;

#include "Scene.h"
#include "gui/MainMenu.h"
#include "input/InputState.h"
#include "object/Hitbox.h"

class App
{
public:
    App(int argc, char* argv[]);
    ~App();

    void run();
    void end();

    auto getTorch() -> trc::TorchStack&;
    auto getAssets() -> trc::AssetManager&;
    auto getScene() -> Scene&;

    // TODO: This is extremely temporary.
    void addHitbox(trc::GeometryID geo, Hitbox hitbox);
    auto getHitbox(trc::GeometryID geo) const -> const Hitbox&;

    static auto get() -> App&;

private:
    static inline App* _app{ nullptr };

    void init();
    void tick();
    bool doEnd{ false };

    u_ptr<int, std::function<void(int*)>> trcTerminator;

    u_ptr<trc::TorchStack> torch;
    u_ptr<trc::imgui::ImguiRenderPass> imgui{ nullptr };
    trc::AssetManager* assetManager;
    Scene scene;

    gui::MainMenu mainMenu;
    InputStateMachine inputState;

    std::unordered_map<trc::GeometryID::LocalID, Hitbox> hitboxes;

    trc::Timer frameTimer;
};
