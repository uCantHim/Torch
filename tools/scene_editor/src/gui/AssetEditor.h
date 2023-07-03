#pragma once

#include <array>
#include <string>

#include <trc/assets/Assets.h>
#include <trc/assets/AssetStorage.h>
#include <trc/text/Font.h>

#include "asset/AssetInventory.h"
#include "asset/HitboxAsset.h"

class App;

namespace gui
{
    class MainMenu;

    class AssetEditor
    {
    public:
        explicit AssetEditor(MainMenu& menu);

        void drawImGui();

    private:
        MainMenu& mainMenu;
        trc::AssetManager& assets;
        AssetInventory& inventory;

        void drawAssetCreateButton();

        void drawAssetList();
        void drawListEntry(const trc::AssetManager::AssetInfo& info);
    };
} // namespace gui
