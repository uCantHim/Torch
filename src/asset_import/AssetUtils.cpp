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

//auto trc::loadScene(
//    const Instance& instance,
//    const fs::path& fbxFilePath,
//    AssetRegistry& assetRegistry
//    ) -> SceneImportResult
//{
//    SceneImportResult result{ .scene=Scene(instance) };
//
//    FBXLoader loader;
//    FileImportData importData = loader.loadFBXFile(fbxFilePath);
//
//    for (const auto& mesh : importData.meshes)
//    {
//        // Load geometry
//        GeometryID geoIdx = assetRegistry.add(GeometryData{ mesh.mesh }, mesh.rig);
//
//        // Load material
//        MaterialID matIdx;
//        if (!mesh.materials.empty()) {
//            matIdx = assetRegistry.add(mesh.materials.front());
//        }
//
//        result.importedGeometries.emplace_back(geoIdx, matIdx);
//
//        // Create drawable
//        Drawable& d = *result.drawables.emplace_back(new Drawable(geoIdx, matIdx));
//        d.attachToScene(result.scene);
//        d.setFromMatrix(mesh.globalTransform);
//    }
//
//    return result;
//}

#endif // #ifdef TRC_USE_FBX_SDK
