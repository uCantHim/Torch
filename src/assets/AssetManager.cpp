#include "trc/assets/AssetManager.h"

#include <source_location>

#include "trc/assets/Assets.h"
#include "trc/assets/DefaultTraits.h"



trc::AssetManager::AssetManager(s_ptr<DataStorage> assetDataStorage)
    :
    dataStorage(assetDataStorage)
{
    assert(assetDataStorage != nullptr);

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

    if (auto trait = getTrait<ManagerTraits>(meta->type)) {
        return trait->get().create(*this, path, dataStorage);
    }
    else {
        return std::nullopt;
    }
}

void trc::AssetManager::destroy(AssetID id)
{
    if (auto trait = getTrait<ManagerTraits>(getAssetType(id))) {
        trait->get().destroy(*this, id);
    }
    else {
        log::warn << "[In " << std::source_location::current().function_name() << "]: "
                  << "Tried to destroy asset, but no implementation for the ManagerTraits trait"
                  << " is registered for asset type " << getAssetType(id).getName() << ".\n";
    }
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
