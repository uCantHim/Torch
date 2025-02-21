#pragma once

#include <trc/assets/AssetManager.h>

#include "asset/AssetInventory.h"
#include "gui/ImguiWindow.h"

class App;

namespace gui
{
    class MainMenu;

    class AssetEditor : public ImguiWindow
    {
    public:
        AssetEditor();

        void drawWindowContent() override;

    private:
        trc::AssetManager& assets;
        AssetInventory& inventory;

        void drawAssetCreateButton();

        void drawAssetList();
        void drawListEntry(const trc::AssetManager::AssetInfo& info);
    };
} // namespace gui
