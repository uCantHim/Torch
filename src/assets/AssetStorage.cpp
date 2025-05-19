#include "trc/assets/AssetStorage.h"

#include "trc/serial/asset.pb.h"



namespace trc
{

AssetStorage::AssetStorage(s_ptr<DataStorage> storage)
    :
    storage(storage)
{
    assert(this->storage != nullptr);
}

bool AssetStorage::isAssetFile(const util::Pathlet& path)
{
    return !!loadFile(path);
}

auto AssetStorage::loadFile(const util::Pathlet& path)
    -> std::expected<serial::AssetFile, std::string>
{
    auto dataStream = storage->read(path);
    if (!dataStream) {
        return std::unexpected("Unable to read from " + path.string() + ".");
    }

    serial::AssetFile file;
    if (!file.ParseFromIstream(dataStream.get())) {
        return std::unexpected("Unable to load asset file data from " + path.string()
                               + ": Parse error.");
    }
    return file;
}

auto AssetStorage::writeFile(const util::Pathlet& path, const serial::AssetFile& file)
    -> std::optional<std::string>
{
    auto dataStream = storage->write(path);
    if (!dataStream) {
        return "Unable to write asset data to " + path.string() + ": System error.";
    }
    if (!file.SerializeToOstream(dataStream.get())) {
        return "Unable to write asset data to " + path.string() + ": Serialization error.";
    }
    return std::nullopt;
}

auto AssetStorage::getMetadata(const AssetPath& path) -> std::optional<AssetMetadata>
{
    auto file = loadFile(path);
    if (!file) {
        return std::nullopt;
    }
    return getMetadata(*file);
}

bool AssetStorage::remove(const AssetPath& path)
{
    return storage->remove(path);
}

auto AssetStorage::getMetadata(const serial::AssetFile& file) -> AssetMetadata
{
    return AssetMetadata::parse(file.metadata());
}

auto AssetStorage::begin() -> iterator
{
    return { storage->begin(), storage->end(), storage };
}

auto AssetStorage::end() -> iterator
{
    return { storage->end(), storage->end(), storage };
}



AssetStorage::AssetIterator::AssetIterator(
    DataStorage::iterator _begin,
    DataStorage::iterator _end,
    s_ptr<DataStorage> _storage)
    :
    iter(std::move(_begin)),
    end(std::move(_end)),
    storage(_storage)
{
    step();
}

auto AssetStorage::AssetIterator::operator*() const -> const_reference
{
    return *currentPath;
}

auto AssetStorage::AssetIterator::operator->() const -> const_pointer
{
    return &*currentPath;
}

auto AssetStorage::AssetIterator::operator++() -> AssetIterator&
{
    ++iter;
    step();
    return *this;
}

bool AssetStorage::AssetIterator::operator==(const AssetIterator& other) const
{
    return iter == other.iter;
}

bool AssetStorage::AssetIterator::isAssetFile(const util::Pathlet& path)
{
    const auto ext = path.extension();
    return ext == ".ta" || AssetStorage{storage}.isAssetFile(path);
}

void AssetStorage::AssetIterator::step()
{
    while (iter != end && !isAssetFile(*iter)) ++iter;
    if (iter != end) {
        currentPath = AssetPath(*iter);
    }
}

} // namespace trc
