#pragma once

#include <array>
#include <string>
#include <vector>

#include <trc/assets/Assets.h>

#include "ImguiUtil.h"
#include "MaterialEditor.h"

class ProjectDirectory;

namespace gui
{
    class AssetEditor
    {
    public:
        AssetEditor(trc::AssetManager& assetManager, ProjectDirectory& assetDir);

        void drawImGui();

    private:
        struct MaterialStorage
        {
            std::string name;
            trc::MaterialID matId;
        };

        trc::AssetManager& assets;
        ProjectDirectory& dir;

        void drawAssetList();
        template<trc::AssetBaseType T>
        void drawListEntry(const trc::AssetPath& path);

        void drawMaterialGui();

        std::array<char, 512> matNameBuf{};
        trc::MaterialID editedMaterial;

        // The edited material is a copy. A click on the save button
        // updates the actual material in the AssetRegistry.
        trc::MaterialData editedMaterialCopy;
    };
} // namespace gui
