#pragma once

#include <unordered_map>
#include <vector>

#include "gui/ImguiUtil.h"
#include "gui/AssetEditor.h"
#include "gui/SceneEditorFileExplorer.h"

class App;

namespace gui
{
    class MainMenu
    {
    public:
        explicit MainMenu(App& app);

        void drawImGui();

        auto getApp() -> App&;
        auto getAssets() -> trc::AssetManager&;

        void openWindow(std::function<bool()> window);

    private:
        void drawMainMenu();

        std::vector<std::function<bool()>> openWindows;

        gui::SceneEditorFileExplorer fileExplorer;
        gui::AssetEditor assetEditor;
    };
} // namespace gui
