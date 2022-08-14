#pragma once

#include <cstdio>

#include <trc/assets/AssetBase.h>
#include <trc/assets/AssetManager.h>

#include "asset/ProjectDirectory.h"

template<trc::AssetBaseType T>
inline bool sanityCheck(const trc::AssetData<T>& data)
{
    std::stringstream ss;
    data.serialize(ss);
    trc::AssetData<T> newData;
    newData.deserialize(ss);

    return std::memcmp(&data, &newData, sizeof(trc::AssetData<T>)) == 0;
}

template<trc::AssetBaseType T>
inline void postProcessing(const trc::AssetData<T>&,
                           const trc::AssetPath&,
                           trc::AssetManager&,
                           ProjectDirectory&)
{
}

/**
 * Import a new asset into the scene editor
 */
template<trc::AssetBaseType T>
inline auto importAsset(const trc::AssetData<T>& data,
                        trc::AssetPath dst,
                        trc::AssetManager& man,
                        ProjectDirectory& dir)
    -> trc::TypedAssetID<T>
{
    // if (!sanityCheck(data)) {
    //     throw std::invalid_argument("Sanity check failed for imported asset data.");
    // }

    // Ensure that the asset path is unique
    int i = 1;
    while (dir.exists(dst)) {
        dst = trc::AssetPath(dst.getUniquePath() + "__" + std::to_string(i++));
    }

    // Create physical storage in ProjectDirectory and logical asset in AssetManager
    dir.save(dst, data);
    const auto id = man.create<T>(dst);

    // Post processing
    postProcessing(data, dst, man, dir);

    return id;
}

/**
 * Specialization for Geometry that automatically creates a hitbox asset
 */
template<>
inline void postProcessing(const trc::AssetData<trc::Geometry>& data,
                           const trc::AssetPath& geoPath,
                           trc::AssetManager& man,
                           ProjectDirectory& dir)
{
    const auto hitbox = makeHitbox(data);
    HitboxData hitboxData{
        .sphere=hitbox.getSphere(),
        .capsule=hitbox.getCapsule(),
        .geometry=geoPath
    };
    const trc::AssetPath path(geoPath.getUniquePath() + "_hitbox");

    dir.save(path, hitboxData);
    man.create<HitboxAsset>(path);
}
