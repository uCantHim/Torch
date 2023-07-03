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
    if (it != pathsToAssets.end())
    {
        destroy(it->second);
        // Path is removed from map in `beforeAssetDestroy`
    }
}

void trc::AssetManager::beforeAssetDestroy(AssetID asset)
{
    try {
        auto path = assetsToPaths.copyAtomically(toIndex(asset));
        pathsToAssets.erase(path);
        assetsToPaths.erase(toIndex(asset));
    }
    catch (const std::out_of_range&) {
        // Asset with the specified ID does not exist (copyAtomically failed).
        // Perhaps I should throw an InvalidAssetIdError here, but I don't know
        // the implications of this so I won't.
    }
}

bool trc::AssetManager::exists(const AssetPath& path) const
{
    return pathsToAssets.contains(path);
}

auto trc::AssetManager::getMetadata(const AssetPath& path) const -> const AssetMetadata*
{
    if (exists(path)) {
        return &getMetadata(pathsToAssets.at(path));
    }
    return nullptr;
}

auto trc::AssetManager::getDataStorage() -> AssetStorage&
{
    return dataStorage;
}
