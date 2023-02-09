#include "trc/util/FilesystemDataStorage.h"

#include <fstream>



namespace trc
{

FilesystemDataStorage::FilesystemDataStorage(const fs::path& rootDir)
    :
    rootDir(rootDir)
{
}

auto FilesystemDataStorage::read(const path& path) -> s_ptr<std::istream>
{
    auto stream = std::make_shared<std::ifstream>(path.filesystemPath(rootDir), std::ios::binary);
    if (stream->is_open()) {
        return stream;
    }
    return nullptr;
}

auto FilesystemDataStorage::write(const path& path) -> s_ptr<std::ostream>
{
    const fs::path fullPath = path.filesystemPath(rootDir);

    if (fs::is_regular_file(fullPath.parent_path())) {
        return nullptr;
    }
    fs::create_directories(fullPath.parent_path());

    auto stream = std::make_shared<std::ofstream>(path.filesystemPath(rootDir), std::ios::binary);
    if (stream->is_open()) {
        return stream;
    }
    return nullptr;
}

bool FilesystemDataStorage::remove(const path& path)
{
    return fs::remove(path.filesystemPath(rootDir));
}

} // namespace trc
