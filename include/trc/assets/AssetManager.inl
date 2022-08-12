#include "AssetManager.h"

#include "import/InternalFormat.h"  // For the loadAssetData specializations



namespace trc
{

template<AssetBaseType T>
inline void AssetManager::resolveReferences(AssetData<T>& data)
{
    if constexpr (requires{ data.resolveReferences(*this); }) {
        data.resolveReferences(*this);
    }
}

template<AssetBaseType T>
inline auto AssetManager::create(const AssetData<T>& data) -> TypedAssetID<T>
{
    auto source = std::make_unique<InMemorySource<T>>(data);
    auto name = source->getAssetName();

    return _createAsset<T>(std::move(source), AssetMetaData{ .name=std::move(name) });
}

template<AssetBaseType T>
inline auto AssetManager::create(u_ptr<AssetSource<T>> source) -> TypedAssetID<T>
{
    return _createAsset<T>(std::move(source), AssetMetaData{ .name=source->getAssetName() });
}

template<AssetBaseType T>
inline auto AssetManager::create(const AssetPath& path) -> TypedAssetID<T>
{
    if (pathsToAssets.contains(path)) {
        throw std::invalid_argument("Asset path \"" + path.getUniquePath() + "\" already exists.");
    }

    const auto id = _createAsset<T>(
        std::make_unique<AssetPathSource<T>>(path),
        AssetMetaData{ .name=path.getUniquePath(), .path=path }
    );
    auto [it, success] = pathsToAssets.try_emplace(path, id);
    assert(success);

    return id;
}

template<AssetBaseType T>
inline void AssetManager::destroy(const TypedAssetID<T> id)
{
    assetIdPool.free(static_cast<AssetID::IndexType>(id.getAssetID()));
    assetMetaData.erase(id.getAssetID());

    registry.remove<T>(id.getDeviceID());
}

template<AssetBaseType T>
inline auto AssetManager::get(const AssetPath& path) const -> TypedAssetID<T>
{
    auto it = pathsToAssets.find(path);
    if (it == pathsToAssets.end())
    {
        throw std::runtime_error("[In AssetManager::getAsset]: Asset at path "
                                 + path.getUniquePath() + " does not exist");
    }

    try {
        return std::any_cast<TypedAssetID<T>>(it->second.typedId);
    }
    catch (const std::bad_any_cast&)
    {
        throw std::runtime_error("[In AssetManager::getAsset]: Asset at path "
                                 + path.getUniquePath() + " does not match the type specified in"
                                 " the function's template parameter");
    }
}

template<AssetBaseType T>
inline auto AssetManager::getMetaData(TypedAssetID<T> id) const -> const AssetMetaData&
{
    return assetMetaData.at(id.getAssetID());
}

template<AssetBaseType T>
inline auto AssetManager::getModule() -> AssetRegistryModule<T>&
{
    return dynamic_cast<AssetRegistryModule<T>&>(registry.getModule<T>());
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
