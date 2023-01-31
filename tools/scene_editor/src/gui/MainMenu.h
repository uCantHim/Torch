#pragma once

#include <unordered_map>
#include <vector>

#include <trc_util/data/DeferredInsertVector.h>

#include "gui/AssetEditor.h"
#include "gui/ImguiUtil.h"
#include "gui/ObjectBrowser.h"
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

        /**
         * @param bool() window Draw function. Returns false when window is
         *        closed, in which case the function gets deleted.
         */
        void openWindow(std::function<bool()> window);

    private:
        void drawMainMenu();

        App& app;

        trc::data::DeferredInsertVector<std::function<bool()>> openWindows;

        gui::SceneEditorFileExplorer fileExplorer;
        gui::AssetEditor assetEditor;
        gui::ObjectBrowser objectBrowser;
    };
} // namespace gui
