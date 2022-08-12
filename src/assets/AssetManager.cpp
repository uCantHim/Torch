#include "assets/AssetManager.h"



trc::AssetManager::AssetManager(const Instance& instance, const AssetRegistryCreateInfo& arInfo)
    :
    registry(instance, arInfo)
{
}

auto trc::AssetManager::getMetaData(const AssetPath& path) const -> const AssetMetaData&
{
    return assetMetaData.at(pathsToAssets.at(path).baseId);
}

void trc::AssetManager::destroy(const AssetPath& path)
{
    auto it = pathsToAssets.find(path);
    if (it != pathsToAssets.end())
    {
        it->second.destroy(*this);
        pathsToAssets.erase(it);
    }
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
