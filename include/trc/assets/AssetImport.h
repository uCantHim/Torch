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

    /**
     * @brief Load geometry from a file directly into an asset registry
     *
     * @param const fs::path& filePath
     * @param bool loadRig Indicate whether a rig should be loaded for the
     *                     geometry if one is found.
     *
     * @return Nothing if any error occurs.
     */
    auto loadGeometry(const fs::path& filePath,
                      AssetRegistry& assetRegistry,
                      bool loadRig = true) -> Maybe<GeometryID>;
} // namespace trc
