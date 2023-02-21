#include "trc/assets/AssetManager.h"

#include "trc/assets/Assets.h"
#include "trc/assets/DefaultTraits.h"



trc::AssetManager::AssetManager(
    s_ptr<DataStorage> assetDataStorage,
    const Instance& instance,
    const AssetRegistryCreateInfo& arInfo)
    :
    registry(instance, arInfo),
    dataStorage(assetDataStorage)
{
    registerDefaultTraits<Material>(assetTraits);
    registerDefaultTraits<Texture>(assetTraits);
    registerDefaultTraits<Geometry>(assetTraits);
    registerDefaultTraits<Rig>(assetTraits);
    registerDefaultTraits<Animation>(assetTraits);
    registerDefaultTraits<Font>(assetTraits);
}

auto trc::AssetManager::getMetaData(const AssetPath& path) const -> const AssetMetadata&
{
    const ui32 index = ui32{pathsToAssets.at(path)};
    return assetInformation.at(index).metadata;
}

void trc::AssetManager::destroy(const AssetPath& path)
{
    auto it = pathsToAssets.find(path);
    if (it != pathsToAssets.end())
    {
        const AssetID id = it->second;
        const AssetType& type = assetInformation.at(ui32{id}).metadata.type;

        getTrait<ManagerTraits>(type).destroy(*this, id);

        assetInformation.erase(ui32{id});
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

auto trc::AssetManager::getAssetStorage() -> AssetStorage&
{
    return dataStorage;
}

auto trc::AssetManager::_createBaseAsset(AssetMetadata meta) -> AssetID
{
    // Generate unique asset ID
    const AssetID id(assetIdPool.generate());

    // Create meta data
    assetInformation.emplace(
        ui32{id},
        AssetInfo{ id, std::any{}, std::move(meta) }
    );

    return id;
}
