#pragma once

#include <componentlib/Table.h>
#include <trc/assets/AssetBase.h>
#include <trc/assets/Geometry.h>

#include "object/Hitbox.h"

class HitboxRegistry;

struct HitboxAsset
{
    using Registry = HitboxRegistry;
    static consteval auto name() -> std::string_view {
        return "hitbox";
    }
};

template<>
class trc::AssetHandle<HitboxAsset> : public Hitbox
{
public:
    AssetHandle(Hitbox hitbox) : Hitbox(hitbox) {}
    AssetHandle(Sphere sphere, Capsule capsule) : Hitbox(sphere, capsule) {}
};

template<>
struct trc::AssetData<HitboxAsset>
{
    Sphere sphere;
    Capsule capsule;

    trc::AssetReference<trc::Geometry> geometry;

    void serialize(std::ostream& os) const;
    void deserialize(std::istream& os);
    void resolveReferences(trc::AssetManager& man);
};

using HitboxData = trc::AssetData<HitboxAsset>;
using HitboxHandle = trc::AssetHandle<HitboxAsset>;

class HitboxRegistry : public trc::AssetRegistryModuleInterface<HitboxAsset>
{
public:
    HitboxRegistry() = default;

    void update(vk::CommandBuffer, trc::FrameRenderState&) override {}

    auto add(u_ptr<trc::AssetSource<HitboxAsset>> source) -> LocalID override;
    void remove(LocalID id) override;
    auto getHandle(LocalID id) -> HitboxHandle override;

    auto getForGeometry(trc::GeometryID geo) -> HitboxHandle;

private:
    trc::data::IdPool<ui32> idPool;
    componentlib::Table<HitboxHandle, LocalID> hitboxes;
    componentlib::Table<LocalID, trc::GeometryID::LocalID> perGeometry;
};
