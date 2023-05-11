#pragma once

#include <cstdio>

#include <trc/assets/AssetBase.h>
#include <trc/assets/AssetManager.h>
#include <trc/assets/Geometry.h>

#include "HitboxAsset.h"

template<trc::AssetBaseType T>
inline void postProcessing(const trc::AssetData<T>&,
                           const trc::AssetPath&,
                           trc::AssetManager&)
{
}

/**
 * Import a new asset into the scene editor
 */
template<trc::AssetBaseType T>
inline auto importAsset(const trc::AssetData<T>& data,
                        const trc::AssetPath& dstPath,
                        trc::AssetManager& man)
    -> trc::TypedAssetID<T>
{
    man.getDataStorage().store(dstPath, data);
    const auto id = man.create<T>(dstPath).value();

    // Post processing
    postProcessing(data, dstPath, man);

    return id;
}

/**
 * Specialization for Geometry that automatically creates a hitbox asset
 */
template<>
inline void postProcessing(const trc::AssetData<trc::Geometry>& data,
                           const trc::AssetPath& geoPath,
                           trc::AssetManager& man)
{
    const auto hitbox = makeHitbox(data);
    HitboxData hitboxData{
        .sphere=hitbox.getSphere(),
        .capsule=hitbox.getCapsule(),
        .geometry=geoPath
    };
    const trc::AssetPath path(geoPath.string() + "_hitbox");

    man.getDataStorage().store(path, hitboxData);
    man.create<HitboxAsset>(path);
}
