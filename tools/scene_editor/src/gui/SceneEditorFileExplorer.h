#pragma once

#include "FileExplorer.h"

namespace gui
{
    class MainMenu;

    class SceneEditorFileExplorer
    {
    public:
        explicit SceneEditorFileExplorer(MainMenu& menu);

        void drawImGui();

    private:
        FileExplorer fileExplorer;
    };
} // namespace gui
