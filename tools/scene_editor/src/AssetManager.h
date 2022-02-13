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
    auto add(trc::Material mat) -> trc::MaterialID;
    auto add(trc::Image image) -> trc::TextureID;
    auto add(trc::Face face) -> trc::Font;
    auto add(trc::AnimationData anim) -> trc::Animation;

    auto get(trc::GeometryID id) -> trc::Geometry;
    auto get(trc::MaterialID id) -> trc::Material&;
    auto get(trc::TextureID id) -> trc::Texture;

    auto getHitbox(trc::GeometryID id) const -> const Hitbox&;

private:
    trc::AssetRegistry* ar;

    std::unordered_map<trc::GeometryID::ID, Hitbox> geometryHitboxes;
};
