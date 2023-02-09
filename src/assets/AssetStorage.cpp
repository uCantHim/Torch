#include "trc/assets/AssetStorage.h"

#include "asset.pb.h"

#include "trc/assets/Assets.h"



namespace trc
{

AssetStorage::AssetStorage(s_ptr<DataStorage> storage)
    :
    storage(storage)
{
    assert(this->storage != nullptr);
}

auto AssetStorage::getMetadata(const AssetPath& path) -> std::optional<AssetMetadata>
{
    auto metaStream = storage->read(makeMetaPath(path));
    if (metaStream != nullptr) {
        return deserializeMetadata(*metaStream);
    }
    return std::nullopt;
}

bool AssetStorage::remove(const AssetPath& path)
{
    return storage->remove(makeMetaPath(path))
        && storage->remove(makeDataPath(path));
}

auto AssetStorage::makeMetaPath(const AssetPath& path) -> util::Pathlet
{
    return util::Pathlet(path.string() + ".meta");
}

auto AssetStorage::makeDataPath(const AssetPath& path) -> util::Pathlet
{
    return util::Pathlet(path.string() + ".data");
}

void AssetStorage::serializeMetadata(const AssetMetadata& meta, std::ostream& os)
{
    serial::AssetMetadata serial;
    serial.set_name(meta.name);
    serial.mutable_type()->set_name(meta.type.getName());
    if (meta.path.has_value()) {
        serial.set_path(meta.path->string());
    }

    serial.SerializeToOstream(&os);
}

auto AssetStorage::deserializeMetadata(std::istream& is) -> AssetMetadata
{
    serial::AssetMetadata serial;
    serial.ParseFromIstream(&is);

    AssetMetadata meta{
        .name=serial.name(),
        .type=AssetType::make(serial.type().name()),
    };
    if (serial.has_path()) {
        meta.path = AssetPath(serial.path());
    }

    return meta;
}

} // namespace trc
