#include "AssetManager.h"



namespace trc
{

template<AssetBaseType T>
inline void AssetManager::resolveReferences(AssetData<T>&)
{
    // Do nothing in the unspecialized version
}

template<>
inline void AssetManager::resolveReferences<Geometry>(AssetData<Geometry>& data)
{
    if (!data.rig.empty()) {
        data.rig.resolve(*this);
    }
}

template<>
inline void AssetManager::resolveReferences<Material>(AssetData<Material>& data)
{
    if (!data.albedoTexture.empty()) {
        data.albedoTexture.resolve(*this);
    }
    if (!data.normalTexture.empty()) {
        data.normalTexture.resolve(*this);
    }
}

template<>
inline void AssetManager::resolveReferences<Rig>(AssetData<Rig>& data)
{
    for (auto& ref : data.animations)
    {
        ref.resolve(*this);
    }
}

template<AssetBaseType T>
inline auto AssetManager::create(const AssetData<T>& data) -> TypedAssetID<T>
{
    return _createAsset<T>(
        std::make_unique<InMemorySource<T>>(data),
        AssetMetaData{ .uniqueName=generateUniqueName() }
    );
}

template<AssetBaseType T>
inline auto AssetManager::create(u_ptr<AssetSource<T>> source) -> TypedAssetID<T>
{
    return _createAsset<T>(
        std::move(source),
        AssetMetaData{ .uniqueName=generateUniqueName() }
    );
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

    registry.remove(id.id);
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
inline auto AssetManager::getAssetMetaData(TypedAssetID<T> id) const -> const AssetMetaData&
{
    return assetMetaData.at(id.getAssetID());
}

template<AssetBaseType T>
inline auto AssetManager::getModule() -> AssetRegistryModule<T>&
{
    return registry.getModule<T>();
}

template<AssetBaseType T>
inline auto AssetManager::_loadAsset(const AssetPath& path) -> TypedAssetID<T>
{
    auto [it, success] = pathsToAssets.try_emplace(path);
    if (!success)
    {
        // Asset from `path` has already been loaded
        return std::any_cast<TypedAssetID<T>>(it->second);
    }

    const auto id = _createAsset<T>(
        std::make_unique<AssetPathSource<T>>(path),
        AssetMetaData{ .uniqueName=path.getUniquePath() }
    );
    it->second = id;

    return id;
}

template<AssetBaseType T>
inline auto AssetManager::_createAsset(u_ptr<AssetSource<T>> source, AssetMetaData meta)
    -> TypedAssetID<T>
{
    // Create general asset information
    const auto assetId = _createBaseAsset(std::move(meta));

    // Create device resource
    u_ptr<AssetSource<T>> internalSource{ new InternalAssetSource<T>(*this, std::move(source)) };
    const auto localId = registry.add(std::move(internalSource));

    return TypedAssetID<T>{ assetId, localId, *this };
}

} // namespace trc
