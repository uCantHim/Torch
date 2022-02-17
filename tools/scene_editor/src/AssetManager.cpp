#include "AssetManager.h"



AssetManager::AssetManager(trc::AssetRegistry& assetRegistry)
    :
    ar(&assetRegistry)
{
}

void AssetManager::updateMaterials()
{
    ar->updateMaterials();
}

auto AssetManager::add(trc::GeometryData data, std::optional<trc::RigData> rig)
    -> trc::GeometryID
{
    auto id = ar->add(data, rig);
    geometryHitboxes.try_emplace(id, makeHitbox(data));

    return id;
}

auto AssetManager::add(trc::Material mat) -> trc::MaterialID
{
    return ar->add(mat);
}

auto AssetManager::add(trc::Image image) -> trc::TextureID
{
    return ar->add(std::move(image));
}

auto AssetManager::get(trc::GeometryID id) -> trc::Geometry
{
    return ar->get(id);
}

auto AssetManager::get(trc::MaterialID id) -> trc::Material&
{
    return ar->get(id);
}

auto AssetManager::get(trc::TextureID id) -> trc::Texture
{
    return ar->get(id);
}

auto AssetManager::add(trc::Face face) -> trc::Font
{
    return trc::Font(ar->getFonts(), std::move(face));
}

auto AssetManager::add(trc::AnimationData anim) -> trc::Animation
{
    return trc::Animation(ar->getAnimations(), anim);
}

auto AssetManager::getHitbox(trc::GeometryID id) const -> const Hitbox&
{
    return geometryHitboxes.at(id);
}
