#pragma once

#include <memory>
#include <vector>
#include <filesystem>

#include "trc_util/Exception.h"
#include "AssimpImporter.h"
#include "FBXImporter.h"
#include "AssetIds.h"

namespace trc
{
    namespace fs = std::filesystem;

    class AssetRegistry;

    class DataImportError : public Exception
    {
    public:
        explicit DataImportError(std::string errorMsg) : Exception(std::move(errorMsg)) {}
    };

    /**
     * @brief Load geometries from a file
     *
     * @throw DataImportError if the file format is not supported
     */
    auto loadGeometry(const fs::path& filePath) -> ThirdPartyFileImportData;

    auto loadTexture(const fs::path& filePath) -> TextureData;
} // namespace trc
