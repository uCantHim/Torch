#pragma once

#include <filesystem>

#include <trc/AssetIds.h>
#include <trc/assets/AssetImportBase.h>

namespace gui
{
    namespace fs = std::filesystem;

    class ImportDialog
    {
    public:
        ImportDialog() = default;
        explicit ImportDialog(const fs::path& filePath);

        void loadFrom(const fs::path& fbxFilePath);

        void drawImGui();

    private:
        static auto importGeometry(const trc::ThirdPartyMeshImport& mesh) -> trc::GeometryID;
        static void importAndCreateObject(const trc::ThirdPartyMeshImport& mesh);

        fs::path filePath;
        trc::ThirdPartyFileImportData importData;
    };
}
