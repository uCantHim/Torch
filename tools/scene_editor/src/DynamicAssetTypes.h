/**
 * Utilities for handling asset types at runtime.
 *
 * One day, I might think about a `registerAssetType(...)`-like mechanism where
 * we can create new types of assets dynamically. For now, they remain hard-coded.
 */
#pragma once

#include <concepts>
#include <stdexcept>

#include <trc/assets/Assets.h>
#include <trc/assets/import/InternalFormat.h>
#include <trc/text/Font.h>

#include "asset/HitboxAsset.h"

enum class AssetType
{
    eGeometry,
    eMaterial,
    eTexture,
    eRig,
    eAnimation,
    eFont,
    eHitbox,

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
    if constexpr (std::same_as<T, trc::Font>)      return AssetType::eFont;
    if constexpr (std::same_as<T, HitboxAsset>)     return AssetType::eHitbox;
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
    case AssetType::eFont:
        vis.template operator()<trc::Font>(std::forward<Args>(args)...);
        break;
    case AssetType::eHitbox:
        vis.template operator()<HitboxAsset>(std::forward<Args>(args)...);
        break;
    default:
        throw std::logic_error("[In fromDynamicType]: Asset type was an unknown enum value."
                               " This is a bug.");
    }
}
