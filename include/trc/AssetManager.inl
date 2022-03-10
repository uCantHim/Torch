#include "AssetManager.h"



namespace trc
{

template<AssetBaseType T>
inline auto AssetManager::createAsset(const AssetData<T>& data) -> TypedAssetID<T>
{
    // Generate unique asset ID
    const AssetID id(assetIdPool.generate());

    // Create meta data
    auto [_, success] = assetMetaData.try_emplace(id, "__no_file_path__");
    assert(success);

    // Create device representation and typed ID
    return { id, registry.add(data), registry.getModule<T>() };
}

template<AssetBaseType T>
inline void AssetManager::destroyAsset(const TypedAssetID<T> id)
{
    assetIdPool.free(static_cast<AssetID::Type>(id.uniqueId));
    assetMetaData.erase(id.uniqueId);

    id.reg->remove(id.id);
    // registry.remove(id.id);
}

} // namespace trc
