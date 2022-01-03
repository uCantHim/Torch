#include "asset_import/AssetUtils.h"



auto trc::loadGeometry(const fs::path& filePath) -> FileImportData
{
    if (filePath.extension() == ".fbx")
    {
#ifdef TRC_USE_FBX_SDK
        FBXLoader loader;
        return loader.loadFBXFile(filePath);
#else
        throw DataImportError("[In loadGeometry]: Unable to import data from .fbx files"
                              " as Torch was built without the FBX SDK");
#endif
    }

#ifdef TRC_USE_ASSIMP
    return AssetImporter::load(filePath);
#else
    throw DataImportError("[In loadGeometry]: Unable to import data from " + filePath.string()
                          + " as Torch was built without Assimp");
#endif
}

auto trc::loadGeometry(
    const fs::path& filePath,
    AssetRegistry& assetRegistry,
    bool loadRig
    ) -> Maybe<GeometryID>
{
    auto loadedMeshes = loadGeometry(filePath).meshes;
    if (loadedMeshes.empty()) {
        return {};
    }

    auto& mesh = loadedMeshes.front();

    return assetRegistry.add(
        mesh.geometry,
        loadRig ? mesh.rig : std::nullopt
    );
}
