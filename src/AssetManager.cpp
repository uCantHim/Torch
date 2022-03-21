#include "AssetManager.h"

#include "GeometryRegistry.h"
#include "TextureRegistry.h"
#include "MaterialRegistry.h"

#include "assets/AssetDataProxy.h"



trc::AssetManager::AssetManager(const Instance& instance, const AssetRegistryCreateInfo& arInfo)
    :
    registry(instance, arInfo)
{
}

auto trc::AssetManager::add(const GeometryData& data) -> GeometryID
{
    return create<Geometry>(data);
}

auto trc::AssetManager::add(const TextureData& data) -> TextureID
{
    return create<Texture>(data);
}

auto trc::AssetManager::add(const MaterialData& data) -> MaterialID
{
    auto ref = data.albedoTexture;
    if (!ref.empty()) {
        ref.resolve(*this);
    }
    ref = data.normalTexture;
    if (!ref.empty()) {
        ref.resolve(*this);
    }

    return create<Material>(data);
}

bool trc::AssetManager::exists(const AssetPath& path) const
{
    return pathsToAssets.contains(path);
}

auto trc::AssetManager::getDeviceRegistry() -> AssetRegistry&
{
    return registry;
}

auto trc::AssetManager::_loadAsset(const AssetPath& path) -> AnyTypedID
{
    auto [it, success] = pathsToAssets.try_emplace(path);
    if (!success)
    {
        // Asset from `path` has already been loaded
        return it->second;
    }

    const AnyTypedID id = _createAsset(AssetDataProxy(path));
    it->second = id;

    return id;
}

auto trc::AssetManager::_createAsset(const AssetDataProxy& data) -> AnyTypedID
{
    const auto assetId = _createBaseAsset(data.getMetaData());

    std::any result = data.visit([this, assetId](const auto& data) -> std::any
    {
        using T = typename decltype(registry.add(data))::Type;

        const auto localId = registry.add(data);
        return TypedAssetID<T>{ assetId, localId, *this };
    });

    return result;
}

auto trc::AssetManager::_createBaseAsset(const AssetMetaData& meta) -> AssetID
{
    // Generate unique asset ID
    const AssetID id(assetIdPool.generate());

    // Create meta data
    auto [_, success] = assetMetaData.try_emplace(id, meta);
    assert(success);

    return id;
}
