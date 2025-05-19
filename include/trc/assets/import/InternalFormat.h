#pragma once

#include "trc/serial/animation.pb.h"
#include "trc/serial/asset.pb.h"
#include "trc/serial/geometry.pb.h"
#include "trc/serial/material.pb.h"
#include "trc/serial/rig.pb.h"
#include "trc/serial/texture.pb.h"

#include "trc/assets/Animation.h"
#include "trc/assets/Geometry.h"
#include "trc/assets/Material.h"
#include "trc/assets/Rig.h"
#include "trc/assets/Texture.h"

namespace trc
{
    namespace internal
    {
        auto serializeAssetData(const AssetData<Geometry>& data) -> serial::Geometry;
        auto serializeAssetData(const AssetData<Texture>& data)  -> serial::Texture;
        auto serializeAssetData(const SimpleMaterialData& data)  -> serial::SimpleMaterial;
        auto serializeAssetData(const AssetData<Rig>& data)  -> serial::Rig;
        auto serializeAssetData(const AssetData<Animation>& data)  -> serial::Animation;

        auto deserializeAssetData(const serial::Geometry& geo) -> GeometryData;
        auto deserializeAssetData(const serial::Texture& tex)  -> TextureData;
        auto deserializeAssetData(const serial::SimpleMaterial& mat) -> SimpleMaterialData;
        auto deserializeAssetData(const serial::Rig& rig) -> RigData;
        auto deserializeAssetData(const serial::Animation& anim) -> AnimationData;
    }
} // namespace trc
