#pragma once

#include <memory>
#include <vector>
#include <filesystem>
namespace fs = std::filesystem;

#include "FBXLoader.h"
#include "AssetRegistry.h"

namespace trc
{
#ifdef TRC_USE_FBX_SDK
    /**
     * @brief Load the first mesh in the file into a geometry
     *
     * @param const fs::path& fbxFilePath
     * @param bool loadRig Indicate whether a rig should be loaded for the
     *                     geometry if one is found.
     *
     * @return Nothing if the file doesn't contain any meshes or a file-I/O
     *         error occurs. Otherwise a Geometry.
     */
    auto loadGeometry(const fs::path& fbxFilePath,
                      AssetRegistry& assetRegistry,
                      bool loadRig = true) -> Maybe<GeometryID>;
#endif
}
