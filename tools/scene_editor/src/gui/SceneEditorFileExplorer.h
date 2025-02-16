#pragma once

#include "gui/FileExplorer.h"
#include "gui/ImguiWindow.h"

namespace gui
{
    class MainMenu;

    class SceneEditorFileExplorer : public ImguiWindow
    {
    public:
        SceneEditorFileExplorer();

        void drawWindowContent() override;

    private:
        FileExplorer fileExplorer;
    };
} // namespace gui
