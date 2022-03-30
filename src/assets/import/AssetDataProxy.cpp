#include "assets/import/AssetDataProxy.h"

#include <trc_util/Util.h>
#include "assets/import/InternalFormat.h"



trc::AssetDataProxy::AssetDataProxy(const AssetPath& path)
{
    load(path);
}

trc::AssetDataProxy::AssetDataProxy(const serial::Asset& asset)
{
    deserialize(asset);
}

trc::AssetDataProxy::AssetDataProxy(AssetDataVariantBase var)
    :
    variant(std::move(var))
{
}

auto trc::AssetDataProxy::getMetaData() const -> const AssetMetaData&
{
    return meta;
}

void trc::AssetDataProxy::load(const AssetPath& path)
{
    deserialize(loadAssetFromFile(path.getFilesystemPath()));
}

void trc::AssetDataProxy::write(const AssetPath& path)
{
    writeAssetToFile(path.getFilesystemPath(), serialize());
}

void trc::AssetDataProxy::deserialize(const serial::Asset& asset)
{
    using Type = serial::Asset::AssetTypeCase;

    switch (asset.asset_type_case())
    {
    case Type::kGeometry:
        variant = deserializeAssetData(asset.geometry());
        break;
    case Type::kTexture:
        variant = deserializeAssetData(asset.texture());
        break;
    case Type::kMaterial:
        variant = deserializeAssetData(asset.material());
        break;
    case Type::kRig:
        variant = deserializeAssetData(asset.rig());
        break;
    case Type::kAnimation:
        variant = deserializeAssetData(asset.animation());
        break;
    default:
        throw std::invalid_argument("[In AssetDataProxy::AssetDataProxy]: Serialized asset data"
                                    " does not contain a valid AssetData type.");
    }
}

auto trc::AssetDataProxy::serialize() const -> serial::Asset
{
    serial::Asset result;
    std::visit(util::VariantVisitor{
        [&](const GeometryData& data)  { *result.mutable_geometry() = serializeAssetData(data); },
        [&](const TextureData& data)   { *result.mutable_texture() = serializeAssetData(data); },
        [&](const MaterialData& data)  { *result.mutable_material() = serializeAssetData(data); },
        [&](const RigData& data)       { *result.mutable_rig() = serializeAssetData(data); },
        [&](const AnimationData& data) { *result.mutable_animation() = serializeAssetData(data); },
    }, variant);

    return result;
}
