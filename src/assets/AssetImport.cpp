#include "assets/AssetImport.h"

#include <vkb/ImageUtils.h>

#include "AssetRegistry.h"



auto trc::loadGeometry(const fs::path& filePath) -> ThirdPartyFileImportData
{
    if (filePath.extension() == ".fbx")
    {
#ifdef TRC_USE_FBX_SDK
        return FBXImporter::load(filePath);
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

auto trc::loadTexture(const fs::path& filePath) -> TextureData
{
    try {
        auto image = vkb::loadImageData2D(filePath);
        return {
            .name   = filePath.filename().replace_extension(""),
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
