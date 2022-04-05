#include "assets/import/AssetImport.h"

#include <vkb/ImageUtils.h>



void linkAssetReferences(trc::ThirdPartyMeshImport& mesh)
{
    if (mesh.rig.has_value())
    {
        auto& rig = mesh.rig.value();
        for (auto& anim : mesh.animations) {
            rig.animations.emplace_back(anim);
        }
        mesh.geometry.rig = rig;
    }
}



auto trc::loadAssets(const fs::path& filePath) -> ThirdPartyFileImportData
{
    auto result = [&]
    {
        if (filePath.extension() == ".fbx")
        {
#ifdef TRC_USE_FBX_SDK
            return FBXImporter::load(filePath);
#else
            throw DataImportError("[In loadAssets]: Unable to import data from .fbx files"
                                  " as Torch was built without the FBX SDK");
#endif
        }

#ifdef TRC_USE_ASSIMP
        return AssetImporter::load(filePath);
#else
        throw DataImportError("[In loadAssets]: Unable to import data from " + filePath.string()
                              + " as Torch was built without Assimp");
#endif
    }();

    for (auto& mesh : result.meshes)
    {
        linkAssetReferences(mesh);
    }

    return result;
}

auto trc::loadGeometry(const fs::path& filePath) -> GeometryData
{
    auto assets = loadAssets(filePath);
    if (assets.meshes.empty()) {
        throw DataImportError("[In loadGeometry]: File does not contain any geometries!");
    }

    auto& mesh = assets.meshes[0];
    linkAssetReferences(mesh);

    return mesh.geometry;
}

auto trc::loadTexture(const fs::path& filePath) -> TextureData
{
    try {
        auto image = vkb::loadImageData2D(filePath);
        return {
            .size   = image.size,
            .pixels = std::move(image.pixels),
        };
    }
    catch (...)
    {
        throw DataImportError("[In loadTexture]: Unable to import texture from "
                              + filePath.string());
    }
}
