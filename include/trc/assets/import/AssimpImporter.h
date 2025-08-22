#pragma once

#include <filesystem>

#include "trc/assets/import/AssetImportBase.h"

namespace trc::import
{
    namespace fs = std::filesystem;

    class AssimpImporter
    {
    public:
        static auto load(const fs::path& filePath) -> std::expected<ThirdPartyImport, ImportError>;
    };
} // namespace trc::import
