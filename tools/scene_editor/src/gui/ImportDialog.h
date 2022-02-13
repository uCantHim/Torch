#pragma once

#include <trc/AssetIds.h>
#include <trc/asset_import/AssetImportBase.h>

namespace gui
{
    class ImportDialog
    {
    public:
        ImportDialog() = default;
        explicit ImportDialog(const fs::path& filePath);

        void loadFrom(const fs::path& fbxFilePath);

        void drawImGui();

    private:
        static auto importGeometry(const trc::Mesh& mesh) -> trc::GeometryID;
        static void importAndCreateObject(const trc::Mesh& mesh);

        fs::path filePath;
        trc::FileImportData importData;
    };
}
