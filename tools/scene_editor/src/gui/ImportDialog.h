#pragma once

#include <filesystem>
#include <string>
#include <unordered_set>

#include <trc/assets/Assets.h>

class App;

namespace gui
{
    namespace fs = std::filesystem;

    class ImportDialog
    {
    public:
        ImportDialog(const fs::path& filePath, App& app);

        void loadFrom(const fs::path& fbxFilePath);

        void drawImGui();

    private:
        auto importGeometry(const trc::ThirdPartyMeshImport& mesh) -> trc::GeometryID;
        void importAndCreateObject(const trc::ThirdPartyMeshImport& mesh);

        App& app;
        fs::path filePath;
        trc::ThirdPartyFileImportData importData;

        /** Flags items as imported to ensure they're not imported multiple times. */
        std::unordered_set<std::string> imported;
    };
}
