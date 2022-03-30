#include "assets/AssetPath.h"

#include "util/TorchDirectories.h"



trc::InvalidAssetPathError::InvalidAssetPathError(fs::path pathlet, std::string error)
    : Exception("Unable to construct unique asset path from \""
                + pathlet.string() + "\": " + error)
{
}



trc::AssetPath::AssetPath(fs::path path)
{
    if (path.empty()) {
        throw InvalidAssetPathError(path, "Path is empty");
    }

    path = path.lexically_normal();

    // Replace any root directory names
    if (path.has_root_path()) {
        path = path.string().replace(0, path.root_path().string().size(), "");
    }

    // Ensure that path is actually a subdirectory of the asset root
    const auto isSubdir = !fs::path(".").lexically_relative(path).empty();
    if (!isSubdir)
    {
        throw InvalidAssetPathError(path, "Path is outside of asset root directory "
                                          + util::getAssetStorageDirectory().string());
    }

    pathlet = std::move(path);
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
    return pathlet.filename().replace_extension("");
}
