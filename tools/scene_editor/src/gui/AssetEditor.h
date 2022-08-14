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
    template<typename U> constexpr const char* assetTypeName{ "" };
    template<> inline constexpr const char* assetTypeName<trc::Material>{ "Material" };
    template<> inline constexpr const char* assetTypeName<trc::Geometry>{ "Geometry" };
    template<> inline constexpr const char* assetTypeName<trc::Texture>{ "Texture" };
    template<> inline constexpr const char* assetTypeName<trc::Rig>{ "Rig" };
    template<> inline constexpr const char* assetTypeName<trc::Animation>{ "Animation" };
    template<> inline constexpr const char* assetTypeName<trc::Font>{ "Font" };
    template<> inline constexpr const char* assetTypeName<HitboxAsset>{ "Hitbox" };

    class AssetEditor
    {
    public:
        explicit AssetEditor(App& app);

        void drawImGui();

    private:
        App& app;
        trc::AssetManager& assets;
        ProjectDirectory& dir;

        void drawAssetList();
        template<trc::AssetBaseType T>
        void drawListEntry(const trc::AssetPath& path);
        template<trc::AssetBaseType T>
        void drawEntryContextMenu(const trc::AssetPath& path);
        void drawDefaultEntryContext(const trc::AssetPath& path);

        void drawMaterialGui();

        std::array<char, 512> matNameBuf{};
        trc::MaterialID editedMaterial;

        // The edited material is a copy. A click on the save button
        // updates the actual material in the AssetRegistry.
        trc::MaterialData editedMaterialCopy;

        void defer(std::function<void(ProjectDirectory&)> func);
        std::vector<std::function<void(ProjectDirectory&)>> deferredFunctions;
    };
} // namespace gui
