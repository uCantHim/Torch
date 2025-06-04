#pragma once

#include <expected>
#include <filesystem>
#include <string>

#include "trc/assets/import/AssetImportBase.h"

namespace trc
{
    namespace fs = std::filesystem;

    class GltfImporter
    {
    public:
        static auto loadFromAsciiFile(const fs::path& filePath)
            -> std::expected<ThirdPartyFileImportData, std::string>;

        static auto loadFromBinaryFile(const fs::path& filePath)
            -> std::expected<ThirdPartyFileImportData, std::string>;

        static auto load(const fs::path& filePath, bool binary = true)
            -> std::expected<ThirdPartyFileImportData, std::string>;
    };
} // namespace trc
