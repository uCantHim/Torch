#pragma once

#include <array>
#include <string>

#include <trc/assets/Assets.h>
#include <trc/assets/AssetStorage.h>
#include <trc/text/Font.h>

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
        App& app;
        MainMenu& mainMenu;
        trc::AssetManager& assets;

        void drawAssetCreateButton();

        void drawAssetList();
        void drawListEntry(const trc::AssetPath& path);
    };
} // namespace gui
