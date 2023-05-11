#include "asset/HitboxAsset.h"

#include "hitbox.pb.h"



void trc::AssetData<HitboxAsset>::serialize(std::ostream& os) const
{
    ::serial::Hitbox data;
    data.set_sphere_radius(sphere.radius);
    data.set_sphere_pos_x(sphere.position.x);
    data.set_sphere_pos_y(sphere.position.y);
    data.set_sphere_pos_z(sphere.position.z);

    data.set_capsule_height(capsule.height);
    data.set_capsule_radius(capsule.radius);
    data.set_capsule_pos_x(capsule.position.x);
    data.set_capsule_pos_y(capsule.position.y);
    data.set_capsule_pos_z(capsule.position.z);

    data.set_geometry_path(geometry.getAssetPath().string());

    data.SerializeToOstream(&os);
}

void trc::AssetData<HitboxAsset>::deserialize(std::istream& is)
{
    ::serial::Hitbox data;
    data.ParseFromIstream(&is);

    sphere.radius = data.sphere_radius();
    sphere.position.x = data.sphere_pos_x();
    sphere.position.y = data.sphere_pos_y();
    sphere.position.z = data.sphere_pos_z();

    capsule.height = data.capsule_height();
    capsule.radius = data.capsule_radius();
    capsule.position.x = data.capsule_pos_x();
    capsule.position.y = data.capsule_pos_y();
    capsule.position.z = data.capsule_pos_z();

    geometry = trc::AssetPath(data.geometry_path());
}

void trc::AssetData<HitboxAsset>::resolveReferences(trc::AssetManager& man)
{
    geometry.resolve(man);
}



auto HitboxRegistry::add(u_ptr<trc::AssetSource<HitboxAsset>> source) -> LocalID
{
    const LocalID id(idPool.generate());
    const auto data = source->load();
    hitboxes.emplace(id, data.sphere, data.capsule);
    perGeometry.emplace(data.geometry.getID().getDeviceID(), id);

    return id;
}

void HitboxRegistry::remove(LocalID id)
{
    hitboxes.erase(id);
    idPool.free(id);
}

auto HitboxRegistry::getHandle(LocalID id) -> HitboxHandle
{
    return hitboxes.get(id);
}

auto HitboxRegistry::getForGeometry(trc::GeometryID geo) -> HitboxHandle
{
    return hitboxes.get(perGeometry.get(geo.getDeviceID()));
}
