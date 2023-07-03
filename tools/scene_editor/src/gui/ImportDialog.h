#pragma once

#include <filesystem>
#include <string>
#include <unordered_set>

#include <trc/assets/Assets.h>

using namespace trc::basic_types;

class App;
class AssetInventory;

namespace gui
{
    namespace fs = std::filesystem;

    /**
     * @brief Loads assets from a file and shows a dialog that allows the user
     *        to import them selectively.
     */
    class ImportDialog
    {
    public:
        explicit ImportDialog(const fs::path& filePath);

        void loadFrom(const fs::path& fbxFilePath);

        void drawImGui();

    private:
        bool importAndCreateObject(const trc::ThirdPartyMeshImport& mesh);
        void createObject(trc::GeometryID geo, mat4 transform);

        fs::path filePath;
        trc::ThirdPartyFileImportData importData;

        /** Flags items as imported to ensure they're not imported multiple times. */
        std::unordered_set<std::string> imported;
    };
}
