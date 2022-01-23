#pragma once

#include <unordered_map>

#include "gui/ImGuiUtil.h"
#include "gui/AssetEditor.h"
#include "gui/SceneEditorFileExplorer.h"

class App;

namespace gui
{
    class MainMenu
    {
    public:
        MainMenu(App& app);

        void drawImGui();

    private:
        void drawMainMenu();

        gui::SceneEditorFileExplorer fileExplorer;
        gui::AssetEditor assetEditor;
    };
} // namespace gui
