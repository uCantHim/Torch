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
    return createAsset<Geometry>(data);
}

auto trc::AssetManager::add(const TextureData& data) -> TextureID
{
    return createAsset<Texture>(data);
}

auto trc::AssetManager::add(const MaterialData& data) -> MaterialID
{
    return createAsset<Material>(data);
}

auto trc::AssetManager::getDeviceRegistry() -> AssetRegistry&
{
    return registry;
}

auto trc::AssetManager::_loadAsset(AssetPath path) -> std::any
{
    return _createAsset(AssetDataProxy(path));
}

auto trc::AssetManager::_createAsset(const AssetDataProxy& data) -> std::any
{
    const auto assetId = _createBaseAsset(data.getMetaData());

    std::any result;
    data.visit([=, this, &result](const auto& data)
    {
        using T = typename decltype(registry.add(data))::Type;

        const auto localId = registry.add(data);
        result = TypedAssetID<T>{ assetId, localId, registry.getModule<T>() };
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
