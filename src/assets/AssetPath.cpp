#include "trc/assets/AssetPath.h"

#include "trc/util/TorchDirectories.h"



trc::AssetPath::AssetPath(fs::path path)
    :
    pathlet(std::move(path))
{
    // Ensure that path is actually a subdirectory of the asset root
    const auto isSubdir = !fs::path{ pathlet.string() }.lexically_relative(fs::path("."))
                           .string().starts_with("..");
    if (!isSubdir)
    {
        throw std::invalid_argument(
            "Unable to construct unique asset path from \"" + path.string() + "\": "
            + "Path is outside of asset root directory "
            + util::getAssetStorageDirectory().string());
    }
}

auto trc::AssetPath::getUniquePath() const -> std::string
{
    return pathlet.string();
}

auto trc::AssetPath::getFilesystemPath() const -> fs::path
{
    return util::getAssetStorageDirectory() / pathlet;
}

auto trc::AssetPath::getAssetName() const -> std::string
{
    return pathlet.filename().replace_extension("").string();
}
