#pragma once

#include <unordered_map>

#include <trc/Scene.h>

#include "gui/ImGuiUtil.h"
#include "gui/AssetEditor.h"
#include "gui/SceneEditorFileExplorer.h"

namespace gui
{
    class MainMenu
    {
    public:
        MainMenu();

        void drawImGui();

    private:
        void drawMainMenu();

        gui::SceneEditorFileExplorer fileExplorer;
        gui::AssetEditor assetEditor;
    };
} // namespace gui
