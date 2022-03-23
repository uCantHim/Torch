#include "AssetManager.h"



namespace trc
{

inline auto generateUniqueName(std::string name)
{
    return "__trc_fileless_asset__" + name;
}



template<AssetBaseType T>
inline auto AssetManager::create(const AssetData<T>& data) -> TypedAssetID<T>
{
    return _createAsset<T>(std::make_unique<InMemorySource<T>>(data));
}

template<AssetBaseType T>
inline auto AssetManager::load(const AssetPath& path) -> TypedAssetID<T>
{
    return _loadAsset<T>(path);
}

template<AssetBaseType T>
inline void AssetManager::destroyAsset(const TypedAssetID<T> id)
{
    assetIdPool.free(static_cast<AssetID::IndexType>(id.uniqueId));
    assetMetaData.erase(id.uniqueId);

    id.reg->remove(id.id);
    // registry.remove(id.id);
}

template<AssetBaseType T>
inline auto AssetManager::getAsset(const AssetPath& path) const -> TypedAssetID<T>
{
    auto it = pathsToAssets.find(path);
    if (it == pathsToAssets.end())
    {
        throw std::runtime_error("[In AssetManager::getAsset]: Asset at path "
                                 + path.getUniquePath() + " does not exist");
    }

    try {
        return std::any_cast<TypedAssetID<T>>(it->second);
    }
    catch (const std::bad_any_cast&)
    {
        throw std::runtime_error("[In AssetManager::getAsset]: Asset at path "
                                 + path.getUniquePath() + " does not match the type specified in"
                                 " the function's template parameter");
    }
}

template<AssetBaseType T>
inline auto AssetManager::getModule() -> AssetRegistryModule<T>&
{
    return registry.getModule<T>();
}

template<AssetBaseType T>
auto AssetManager::_loadAsset(const AssetPath& path) -> TypedAssetID<T>
{
    auto [it, success] = pathsToAssets.try_emplace(path);
    if (!success)
    {
        // Asset from `path` has already been loaded
        return std::any_cast<TypedAssetID<T>>(it->second);
    }

    const auto id = _createAsset<T>(std::make_unique<AssetPathSource<T>>(path));
    it->second = id;

    return id;
}

template<AssetBaseType T>
inline auto AssetManager::_createAsset(u_ptr<AssetSource<T>> source) -> TypedAssetID<T>
{
    const auto assetId = _createBaseAsset({});

    const auto localId = registry.add(std::move(source));
    return TypedAssetID<T>{ assetId, localId, *this };
}

} // namespace trc
