#include "asset/ProjectDirectory.h"

#include <trc/util/TorchDirectories.h>



ProjectDirectory::ProjectDirectory(const fs::path& root)
    :
    root(root)
{
    if (!fs::is_directory(root)) {
        throw std::invalid_argument("[In ProjectDirectory::ProjectDirectory]: "
                                    + root.string() + " is not a directory.");
    }

    try {
        index.load(root / ".assetIndex.json");
    } catch (const std::runtime_error&) {}
}

ProjectDirectory::~ProjectDirectory()
{
    index.save(root / ".assetIndex.json");
}

bool ProjectDirectory::exists(const trc::AssetPath& path) const
{
    return index.contains(path);
}

void ProjectDirectory::move(const trc::AssetPath& from, const trc::AssetPath& to)
{
    if (!index.contains(from)) {
        throw std::invalid_argument(from.string() + " is not a registered asset.");
    }
    if (index.contains(to)) {
        throw std::invalid_argument("Asset already exists at " + to.string());
    }

    const auto type = *index.getType(from);
    // Remove old index entry
    index.erase(from);
    // Add new index entry
    fromDynamicType([this, &to]<trc::AssetBaseType T>(){ index.insert<T>(to); }, type);

    fs::rename(getFilesystemPath(from), getFilesystemPath(to));
}

void ProjectDirectory::remove(const trc::AssetPath& path)
{
    if (!index.contains(path)) {
        throw std::out_of_range(path.string() + " is not a registered asset.");
    }

    std::scoped_lock lock(fileWriteLock);
    index.erase(path);
    fs::remove(getFilesystemPath(path));
}

auto ProjectDirectory::getFilesystemPath(const trc::AssetPath& path) -> fs::path
{
    return trc::util::getAssetStorageDirectory() / path;
}
