#pragma once

#include <functional>

#include <trc_util/data/DeferredInsertVector.h>

#include "gui/SceneEditorFileExplorer.h"

namespace gui
{
    class MainMenu
    {
    public:
        MainMenu();

        void drawImGui();

        /**
         * @param bool() window Draw function. Returns false when window is
         *        closed, in which case the function gets deleted.
         */
        void openWindow(std::function<bool()> window);

    private:
        void drawMainMenu();

        gui::SceneEditorFileExplorer fileExplorer;
        trc::data::DeferredInsertVector<std::function<bool()>> openWindows;
    };
} // namespace gui
