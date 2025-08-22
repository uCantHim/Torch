#pragma once

#include <expected>
#include <filesystem>

#include "trc/assets/import/AssetImportBase.h"

namespace trc::import
{
    namespace fs = std::filesystem;

    class GltfImporter
    {
    public:
        static auto loadFromAsciiFile(const fs::path& filePath)
            -> std::expected<import::ThirdPartyImport, import::ImportError>;

        static auto loadFromBinaryFile(const fs::path& filePath)
            -> std::expected<import::ThirdPartyImport, import::ImportError>;

        static auto load(const fs::path& filePath, bool binary = true)
            -> std::expected<import::ThirdPartyImport, import::ImportError>;
    };
} // namespace trc::import
