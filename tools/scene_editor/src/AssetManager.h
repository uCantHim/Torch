#pragma once

#include <unordered_map>

#include <trc/AssetRegistry.h>

#include "object/Hitbox.h"

/**
 * @brief
 */
class AssetManager
{
public:
    explicit AssetManager(trc::AssetRegistry& assetRegistry);

    void updateMaterials();

    auto add(trc::GeometryData data, std::optional<trc::RigData> rig = {}) -> trc::GeometryID;
    auto add(trc::MaterialDeviceHandle mat) -> trc::MaterialID;
    auto add(trc::TextureData tex) -> trc::TextureID;
    auto add(trc::Face face) -> trc::Font;
    auto add(trc::AnimationData anim) -> trc::Animation;

    auto get(trc::GeometryID id) -> trc::GeometryDeviceHandle;
    auto get(trc::MaterialID id) -> trc::MaterialDeviceHandle&;
    auto get(trc::TextureID id) -> trc::TextureDeviceHandle;

    auto getHitbox(trc::GeometryID id) const -> const Hitbox&;

private:
    trc::AssetRegistry* ar;

    std::unordered_map<trc::GeometryID::ID, Hitbox> geometryHitboxes;
};
