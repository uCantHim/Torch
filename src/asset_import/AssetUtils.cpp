#include "asset_import/AssetUtils.h"


#ifdef TRC_USE_FBX_SDK

auto trc::loadGeometry(
    const fs::path& fbxFilePath,
    AnimationDataStorage& animStorage,
    bool loadRig
    ) -> Maybe<Geometry>
{
    FBXLoader loader;
    auto loadedMeshes = loader.loadFBXFile(fbxFilePath).meshes;
    if (loadedMeshes.empty()) {
        return {};
    }

    auto& mesh = loadedMeshes.front();
    if (loadRig && mesh.rig.has_value())
    {
        return Geometry{
            mesh.mesh,
            std::make_unique<Rig>(std::move(mesh.rig.value()),
                                  animStorage,
                                  std::move(mesh.animations))
        };
    }
    else {
        return Geometry{ mesh.mesh };
    }
}

auto trc::loadScene(
    const Instance& instance,
    const fs::path& fbxFilePath,
    AnimationDataStorage& animStorage
    ) -> SceneImportResult
{
    SceneImportResult result{ .scene=Scene(instance) };

    FBXLoader loader;
    FileImportData importData = loader.loadFBXFile(fbxFilePath);

    for (const auto& mesh : importData.meshes)
    {
        // Load geometry
        std::unique_ptr<Rig> rig{ nullptr };
        if (mesh.rig.has_value()) {
            rig = std::make_unique<Rig>(std::move(mesh.rig.value()),
                                        animStorage,
                                        std::move(mesh.animations));
        }
        GeometryID geoIdx = AssetRegistry::addGeometry({ mesh.mesh, std::move(rig) });

        // Load material
        MaterialID matIdx{ 0 };
        if (!mesh.materials.empty())
        {
            matIdx = AssetRegistry::addMaterial(mesh.materials.front());
        }

        result.importedGeometries.emplace_back(geoIdx, matIdx);

        // Create drawable
        Drawable& d = *result.drawables.emplace_back(new Drawable(geoIdx, matIdx, result.scene));
        d.setFromMatrix(mesh.globalTransform);
    }

    AssetRegistry::updateMaterials();

    return result;
}

#endif // #ifdef TRC_USE_FBX_SDK
