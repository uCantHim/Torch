#include "assets/AssetPath.h"

#include "util/TorchDirectories.h"



trc::AssetPath::AssetPath(fs::path path)
{
    if (path.empty()) {
        throw std::invalid_argument("Unable to construct unique asset path from \""
                                    + path.string() + "\": " + "Path is empty.");
    }

    if (path == "."
        || path.string().ends_with("/")
        || path.string().ends_with(fs::path::preferred_separator))
    {
        throw std::invalid_argument("Unable to construct unique asset path from \""
                                    + path.string() + "\": " + "Path must not be a directory name.");
    }

    // Replace any root directory names
    if (path.has_root_path()) {
        path = path.string().substr(path.root_path().string().size());
    }

    path = path.lexically_normal();

    // Ensure that path is actually a subdirectory of the asset root
    const auto isSubdir = !path.lexically_relative(fs::path("."))
                           .string().starts_with("..");
    if (!isSubdir)
    {
        throw std::invalid_argument(
            "Unable to construct unique asset path from \"" + path.string() + "\": "
            + "Path is outside of asset root directory "
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
