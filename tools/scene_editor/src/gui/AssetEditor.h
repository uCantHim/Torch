#pragma once

#include <vector>
#include <unordered_set>

#include "ImguiUtil.h"
#include "AssetManager.h"
#include "MaterialEditor.h"

namespace gui
{
    class AssetEditor
    {
    public:
        AssetEditor(AssetManager& assetManager);

        void drawImGui();

    private:
        struct MaterialStorage
        {
            std::string name;
            trc::MaterialID matId;
        };

        AssetManager* assets;

        void drawMaterialGui();
        void drawMaterialList();
        std::array<char, 512> matNameBuf{};
        std::vector<MaterialStorage> materials;

        trc::MaterialID editedMaterial;

        // The edited material is a copy. A click on the save button
        // updates the actual material in the AssetRegistry.
        trc::Material editedMaterialCopy;
    };
} // namespace gui
