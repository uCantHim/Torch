#pragma once

#include <array>
#include <string>

#include <trc/assets/Assets.h>
#include <trc/text/Font.h>

#include "asset/HitboxAsset.h"

class ProjectDirectory;

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
        ProjectDirectory& dir;

        void drawAssetList();
        template<trc::AssetBaseType T>
        void drawListEntry(const trc::AssetPath& path);
        template<trc::AssetBaseType T>
        void drawEntryContextMenu(const trc::AssetPath& path);
        void drawDefaultEntryContext(const trc::AssetPath& path);

        void drawMaterialGui();
        void editMaterial(trc::MaterialID mat);

        std::vector<std::pair<std::string, std::function<void()>>> assetCreateMenu;

        void defer(std::function<void(ProjectDirectory&)> func);
        std::vector<std::function<void(ProjectDirectory&)>> deferredFunctions;
    };
} // namespace gui
