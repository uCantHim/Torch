#include "AssetManager.h"

#include "assets/AssetDataProxy.h"



namespace trc
{

inline auto generateUniqueName(std::string name)
{
    return "__trc_fileless_asset__" + name;
}



template<AssetBaseType T>
inline auto AssetManager::create(const AssetData<T>& data) -> TypedAssetID<T>
{
    try {
        auto res = _createAsset(AssetDataProxy{ data });
        return std::any_cast<TypedAssetID<T>>(res);
    }
    catch(const std::bad_any_cast&)
    {
        throw std::logic_error(
            "[In AssetManager::createAsset<>]: Internal engine logic error: Imported asset data"
            " type did not result in the correct TypedAssetID specialization returned as a"
            " std::any from createAsset(AssetDataProxy)! This result should not be possible."
        );
    }
}

template<AssetBaseType T>
inline auto AssetManager::load(const AssetPath& path) -> TypedAssetID<T>
{
    try {
        return std::any_cast<TypedAssetID<T>>(_loadAsset(path));
    }
    catch(const std::bad_any_cast&)
    {
        throw std::invalid_argument(
            "[In AssetManager::loadAsset<>]: Asset imported from " + path.getUniquePath()
            + " (full path " + path.getFilesystemPath().string() + ") does not match the type"
            " specified in the function's template parameter."
        );
    }
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

} // namespace trc
