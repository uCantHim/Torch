#include "trc/assets/import/AssetImport.h"

#include "trc/base/ImageUtils.h"
#include "trc/base/Logging.h"



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
    auto result = [&]() -> ThirdPartyFileImportData
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

        return {};
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
        auto image = loadImageData2D(filePath);
        return {
            .size   = image.size,
            .pixels = std::move(image.pixels),
        };
    }
    catch (const std::runtime_error& err)
    {
        throw DataImportError("[In loadTexture]: Unable to import texture from "
                              + filePath.string() + ": " + err.what());
    }
}

auto trc::loadFont(const fs::path& path, ui32 fontSize) -> AssetData<Font>
{
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open())
    {
        log::error << log::here() << ": Unable to import font from file " << path
                   << " because the file cannot be opened in read-only mode.";
        throw DataImportError("Unable to read from file " + path.string());
    }

    std::stringstream ss;
    ss << file.rdbuf();
    auto data = ss.str();

    std::vector<std::byte> _data(data.size());
    memcpy(_data.data(), data.data(), data.size());

    return { .fontSize=fontSize, .fontData=std::move(_data) };
}
