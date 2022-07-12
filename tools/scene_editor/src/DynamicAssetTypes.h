#pragma once

#include <concepts>
#include <stdexcept>

#include <trc/assets/AssetBaseTypes.h>
#include <trc/assets/import/InternalFormat.h>

enum class AssetType
{
    eGeometry,
    eMaterial,
    eTexture,
    eRig,
    eAnimation,

    eMaxEnum
};

template<trc::AssetBaseType T>
constexpr auto toDynamicType() -> AssetType
{
    if constexpr (std::same_as<T, trc::Geometry>)  return AssetType::eGeometry;
    if constexpr (std::same_as<T, trc::Material>)  return AssetType::eMaterial;
    if constexpr (std::same_as<T, trc::Texture>)   return AssetType::eTexture;
    if constexpr (std::same_as<T, trc::Rig>)       return AssetType::eRig;
    if constexpr (std::same_as<T, trc::Animation>) return AssetType::eAnimation;
}

template<typename Visitor, typename... Args>
constexpr void fromDynamicType(Visitor&& vis, AssetType type, Args&&... args)
{
    switch (type)
    {
    case AssetType::eGeometry:
        vis.template operator()<trc::Geometry>(std::forward<Args>(args)...);
        break;
    case AssetType::eMaterial:
        vis.template operator()<trc::Material>(std::forward<Args>(args)...);
        break;
    case AssetType::eTexture:
        vis.template operator()<trc::Texture>(std::forward<Args>(args)...);
        break;
    case AssetType::eRig:
        vis.template operator()<trc::Rig>(std::forward<Args>(args)...);
        break;
    case AssetType::eAnimation:
        vis.template operator()<trc::Animation>(std::forward<Args>(args)...);
        break;
    default:
        throw std::logic_error("[In fromDynamicType]: Asset type was an unknown enum value."
                               " This is a bug.");
    }
}

/**
 * @brief Load asset data from any type
 */
template<typename Visitor>
inline void deserializeAssetData(Visitor&& vis, const trc::serial::Asset& asset)
{
    vis.template operator()<trc::Geometry>();

    switch (asset.asset_type_case())
    {
    case trc::serial::Asset::AssetTypeCase::kGeometry:
        vis(trc::deserializeAssetData(asset.geometry()));
        break;
    case trc::serial::Asset::AssetTypeCase::kMaterial:
        vis(trc::deserializeAssetData(asset.material()));
        break;
    case trc::serial::Asset::AssetTypeCase::kTexture:
        vis(trc::deserializeAssetData(asset.texture()));
        break;
    case trc::serial::Asset::AssetTypeCase::kRig:
        vis(trc::deserializeAssetData(asset.rig()));
        break;
    case trc::serial::Asset::AssetTypeCase::kAnimation:
        vis(trc::deserializeAssetData(asset.animation()));
        break;
    case trc::serial::Asset::AssetTypeCase::ASSET_TYPE_NOT_SET:
        throw std::logic_error("[In deserializeAssetData]: Asset type was ASSET_TYPE_NOT_SET."
                               " This is a bug.");
    default:
        throw std::logic_error("[In deserializeAssetData]: Asset type was an unknown enum value."
                               " This is a bug.");
    }
}
