#pragma once

#include <memory>

#include <trc/Torch.h>
using namespace trc::basic_types;

#include "Scene.h"
#include "gui/ImGuiUtil.h"
#include "gui/MainMenu.h"

class App
{
public:
    static void start(int argc, char* argv[]);
    static void end();

    static auto getTorch() -> trc::TorchStack&;
    static auto getAssets() -> trc::AssetRegistry&;
    static auto getScene() -> Scene&;

private:
    static void init();
    static void tick();
    static void terminate();
    static inline bool doEnd{ false };

    static inline u_ptr<trc::TorchStack> torch;
    static inline u_ptr<Scene> scene{ nullptr };
    static inline u_ptr<trc::imgui::ImguiRenderPass> imgui{ nullptr };

    static inline gui::MainMenu mainMenu;
};
