#include "trc/assets/import/AssetImport.h"

#include <fstream>

#include "trc/assets/import/AssimpImporter.h"
#include "trc/assets/import/FBXImporter.h"
#include "trc/assets/import/GltfImporter.h"
#include "trc/base/ImageUtils.h"
#include "trc/base/Logging.h"



namespace trc
{

auto importAssets(const fs::path& filePath) -> std::expected<import::ThirdPartyImport, import::ImportError>
{
    auto result = [&]() -> std::expected<import::ThirdPartyImport, import::ImportError>
    {
        if (filePath.extension() == ".fbx")
        {
#ifdef TRC_USE_FBX_SDK
            log::warn << log::here() << ": "
                      << "Loading from " << filePath << " - the FBX importer is deprecated."
                         " We prefer glTF over FBX because it doesn't require proprietary"
                         " software to be used in Torch.";
            return FBXImporter::load(filePath);
#else
            log::warn << log::here() << ": "
                      << "Loading data from an .fbx file (" << filePath << ") when Torch was not"
                         " built with the FBX SDK enabled. The fallback asset importer may not be"
                         " able to load all asset data from the file.";
#endif
        }

        if (filePath.extension() == ".glb") {
            return import::GltfImporter::loadFromBinaryFile(filePath);
        }
        if (filePath.extension() == ".gltf") {
            return import::GltfImporter::loadFromAsciiFile(filePath);
        }

#ifdef TRC_USE_ASSIMP
        return import::AssimpImporter::load(filePath);
#endif

        return std::unexpected(import::ImportError{
            filePath,
            import::ImportError::Code::eNotSupported,
            "[In loadAssets]: Unable to import data from " + filePath.string() + ";"
            " Torch was not built with any asset importers enabled."
            " Install Assimp for Torch to find it during build or set the CMake option"
            " `TORCH_USE_FBX_SDK=ON` to build Torch with an installed FBX SDK to enable the"
            " FBX importer."
        });
    }();

    if (result) {
        result->bakeAssociations();
    }

    return result;
}

auto importGeometry(const fs::path& filePath) -> std::expected<GeometryData, import::ImportError>
{
    auto assets = importAssets(filePath);
    if (assets->geometries.empty()) {
        return std::unexpected(import::ImportError{
            filePath,
            import::ImportError::Code::eOther,
            "[In loadGeometry]: File does not contain any geometries."
        });
    }

    return assets->geometries.front().data;
}

auto importGeometry(const fs::path& filePath, std::string_view name)
    -> std::expected<GeometryData, import::ImportError>
{
    auto assets = importAssets(filePath);
    if (!assets) {
        return std::unexpected(assets.error());
    }
    if (auto mesh = assets->findMesh(name)) {
        return mesh->data;
    }

    return std::unexpected(import::ImportError{
        filePath,
        import::ImportError::Code::eOther,
        std::format("[In loadGeometry]: File does not contain a geometry with the name \"{}\".", name)
    });
}

auto importTexture(const fs::path& filePath) -> std::expected<TextureData, import::ImportError>
{
    try {
        auto image = loadImageData2D(filePath);
        return TextureData{
            .size   = image.size,
            .pixels = std::move(image.pixels),
        };
    }
    catch (const std::runtime_error& err)
    {
        return std::unexpected(import::ImportError{
            filePath,
            import::ImportError::Code::eOther,
            "[In loadTexture]: Unable to import texture from " + filePath.string()
            + ": " + err.what()
        });
    }
}

auto importFont(const fs::path& path, ui32 fontSize) -> std::expected<AssetData<Font>, import::ImportError>
{
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open())
    {
        log::error << log::here() << ": Unable to import font from file " << path
                   << " because the file cannot be opened in read-only mode.";
        return std::unexpected(import::ImportError{
            path,
            import::ImportError::Code::eFilesystemError,
            "Unable to read from file " + path.string()
        });
    }

    std::stringstream ss;
    ss << file.rdbuf();
    auto data = ss.str();

    std::vector<std::byte> _data(data.size());
    memcpy(_data.data(), data.data(), data.size());

    return FontData{ .fontSize=fontSize, .fontData=std::move(_data) };
}

} // namespace trc
