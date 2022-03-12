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
     * @brief Load all assets from a geometry file type
     *
     * Tries to interpret the file as a geometry or scene data file type,
     * such as COLLADA, FBX, OBJ, ...
     *
     * @throw DataImportError if the file format is not supported
     */
    auto loadAssets(const fs::path& filePath) -> ThirdPartyFileImportData;

    /**
     * @brief Load the first geometry from a file
     *
     * Tries to interpret the file as a geometry storing file type, such
     * as COLLADA, FBX, or OBJ.
     *
     * @throw DataImportError if the file format is not supported
     */
    auto loadGeometry(const fs::path& filePath) -> GeometryData;

    auto loadTexture(const fs::path& filePath) -> TextureData;
} // namespace trc
