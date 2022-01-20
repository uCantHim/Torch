#pragma once

#include <trc/asset_import/FBXLoader.h>

namespace gui
{
    class FbxImportDialog
    {
    public:
        FbxImportDialog() = default;
        explicit FbxImportDialog(const fs::path& filePath);

        void loadFrom(const fs::path& fbxFilePath);

        void drawImGui();

    private:
        fs::path filePath;
        trc::FileImportData importData;
    };
}
