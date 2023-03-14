#include "trc/assets/AssetManager.h"

#include "trc/assets/Assets.h"
#include "trc/assets/DefaultTraits.h"



trc::AssetManager::AssetManager(s_ptr<DataStorage> assetDataStorage)
    :
    dataStorage(assetDataStorage)
{
    registerDefaultTraits<Material>(assetTraits);
    registerDefaultTraits<Texture>(assetTraits);
    registerDefaultTraits<Geometry>(assetTraits);
    registerDefaultTraits<Rig>(assetTraits);
    registerDefaultTraits<Animation>(assetTraits);
    registerDefaultTraits<Font>(assetTraits);
}

auto trc::AssetManager::getMetadata(const AssetPath& path) const -> const AssetMetadata&
{
    return getMetadata(pathsToAssets.at(path));
}

auto trc::AssetManager::create(const AssetPath& path) -> std::optional<AssetID>
{
    const auto meta = dataStorage.getMetadata(path);
    if (!meta.has_value()) {
        return std::nullopt;
    }

    return getTrait<ManagerTraits>(meta->type).create(*this, path, dataStorage);
}

void trc::AssetManager::destroy(AssetID id)
{
    getTrait<ManagerTraits>(getAssetType(id)).destroy(*this, id);
}

void trc::AssetManager::destroy(const AssetPath& path)
{
    auto it = pathsToAssets.find(path);
    if (it != pathsToAssets.end())
    {
        destroy(it->second);
        pathsToAssets.erase(it);
    }
}

bool trc::AssetManager::exists(const AssetPath& path) const
{
    return pathsToAssets.contains(path);
}

auto trc::AssetManager::getDataStorage() -> AssetStorage&
{
    return dataStorage;
}
