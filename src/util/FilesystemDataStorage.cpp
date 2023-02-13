#include "trc/util/FilesystemDataStorage.h"

#include <fstream>



namespace trc
{

FilesystemDataStorage::FilesystemDataStorage(const fs::path& rootDir)
    :
    rootDir(rootDir)
{
    if (!fs::is_directory(rootDir))
    {
        throw std::invalid_argument("[In FilesystemDataStorage::FilesystemDataStorage()]: "
                                    + rootDir.string() + " is not a directory.");
    }
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

auto FilesystemDataStorage::begin() -> iterator
{
    return iterator{ std::make_unique<FileIterator>(rootDir) };
}

auto FilesystemDataStorage::end() -> iterator
{
    return iterator{ std::make_unique<FileIterator>(FileIterator::makeEnd(rootDir)) };
}



FilesystemDataStorage::FileIterator::FileIterator(const fs::path& dir)
    :
    rootDir(dir),
    iter(dir)
{
    assert(fs::is_directory(dir));

    while (iter != fs::end(iter) && iter->is_directory()) ++iter;
    if (iter != fs::end(iter)) {
        current = util::Pathlet(fs::relative(iter->path(), rootDir));
    }
}

FilesystemDataStorage::FileIterator::FileIterator(
    const fs::path& rootDir,
    fs::recursive_directory_iterator iter)
    :
    rootDir(rootDir),
    iter(std::move(iter))
{
}

auto FilesystemDataStorage::FileIterator::operator*() -> reference
{
    return *current;
}

auto FilesystemDataStorage::FileIterator::operator*() const -> const_reference
{
    return *current;
}

auto FilesystemDataStorage::FileIterator::operator->() -> pointer
{
    return &*current;
}

auto FilesystemDataStorage::FileIterator::operator->() const -> const_pointer
{
    return &*current;
}

auto FilesystemDataStorage::FileIterator::operator++() -> EntryIterator&
{
    while (++iter != fs::end(iter) && iter->is_directory());
    if (iter != fs::end(iter)) {
        current = util::Pathlet(fs::relative(iter->path(), rootDir));
    }
    return *this;
}

bool FilesystemDataStorage::FileIterator::operator==(const EntryIterator& _other) const
{
    auto other = dynamic_cast<const FileIterator*>(&_other);
    return other != nullptr
        && iter == other->iter
        && rootDir == other->rootDir;
}

auto FilesystemDataStorage::FileIterator::makeEnd(const fs::path& rootDir) -> FileIterator
{
    return { rootDir, fs::end(fs::recursive_directory_iterator(rootDir)) };
}

} // namespace trc
