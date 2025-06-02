#include "trc/assets/AssetManager.h"

#include <source_location>



trc::AssetManager::AssetManager(s_ptr<DataStorage> assetDataStorage)
    :
    dataStorage(assetDataStorage)
{
    assert(assetDataStorage != nullptr);
}

auto trc::AssetManager::create(const AssetPath& path) -> std::optional<AssetID>
{
    const auto meta = dataStorage.getMetadata(path);
    if (!meta.has_value()) {
        return std::nullopt;
    }

    if (auto trait = getTrait<ManagerTraits>(meta->type))
    {
        assert(trait != nullptr);
        return trait->create(*this, path);
    }
    else {
        throw std::out_of_range(
            "[In " + std::string(std::source_location::current().function_name()) + "]:"
            " asset data at the specified path (" + path.string() + ") is of type "
            + meta->type.getName() + ", which is not registered at the asset manager."

            " Specifically, an implementation of the ManagerTraits asset trait must be specified"
            " for any asset type that the manager is supposed to handle. Use"
            " `AssetManager::registerAssetType` to register this trait automatically (recommended)"
            " or `AssetManager::registerTrait` to register an asset trait manually. The latter"
            " option is only recommended for custom asset traits."
        );
    }
}

void trc::AssetManager::destroy(AssetID id)
{
    if (auto trait = getTrait<ManagerTraits>(getAssetType(id)))
    {
        assert(trait != nullptr);
        trait->destroy(*this, id);
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
    if (it != pathsToAssets.end()) {
        destroy(it->second);
    }
}

bool trc::AssetManager::exists(const AssetPath& path) const
{
    return pathsToAssets.contains(path);
}

auto trc::AssetManager::getAssetType(AssetID id) const -> const AssetType&
{
    return base.getAssetType(id);
}

auto trc::AssetManager::getMetadata(const AssetPath& path) const -> const AssetMetadata*
{
    if (exists(path)) {
        return &getMetadata(pathsToAssets.at(path));
    }
    return nullptr;
}

auto trc::AssetManager::begin() const -> const_iterator
{
    return base.begin();
}

auto trc::AssetManager::end() const -> const_iterator
{
    return base.end();
}

auto trc::AssetManager::getDataStorage() -> AssetStorage&
{
    return dataStorage;
}

auto trc::AssetManager::getDeviceRegistry() -> AssetRegistry&
{
    return base.getDeviceRegistry();
}
