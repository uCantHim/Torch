#include "asset_import/AssetUtils.h"


#ifdef TRC_USE_FBX_SDK

auto trc::loadGeometry(
    const fs::path& fbxFilePath,
    AssetRegistry& assetRegistry,
    bool loadRig
    ) -> Maybe<GeometryID>
{
    FBXLoader loader;
    auto loadedMeshes = loader.loadFBXFile(fbxFilePath).meshes;
    if (loadedMeshes.empty()) {
        return {};
    }

    auto& mesh = loadedMeshes.front();

    return assetRegistry.add(
        mesh.mesh,
        loadRig ? mesh.rig : std::nullopt
    );
}

#endif // #ifdef TRC_USE_FBX_SDK
