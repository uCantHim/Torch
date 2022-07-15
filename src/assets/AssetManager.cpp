#include "assets/AssetManager.h"



trc::AssetManager::AssetManager(const Instance& instance, const AssetRegistryCreateInfo& arInfo)
    :
    registry(instance, arInfo)
{
}

bool trc::AssetManager::exists(const AssetPath& path) const
{
    return pathsToAssets.contains(path);
}

auto trc::AssetManager::getDeviceRegistry() -> AssetRegistry&
{
    return registry;
}

auto trc::AssetManager::_createBaseAsset(AssetMetaData meta) -> AssetID
{
    // Generate unique asset ID
    const AssetID id(assetIdPool.generate());

    // Create meta data
    auto [_, success] = assetMetaData.try_emplace(id, std::move(meta));
    assert(success);

    return id;
}
