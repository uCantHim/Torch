#include "asset/HitboxAsset.h"



void trc::AssetData<HitboxAsset>::serialize(std::ostream& ) const
{
}

void trc::AssetData<HitboxAsset>::deserialize(std::istream& )
{
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
    perGeometry.emplace(data.geometry.getID(), id);

    return id;
}

void HitboxRegistry::remove(LocalID id)
{
    hitboxes.erase(id);
}

auto HitboxRegistry::getHandle(LocalID id) -> HitboxHandle
{
    return hitboxes.get(id);
}

auto HitboxRegistry::getForGeometry(trc::GeometryID geo) -> HitboxHandle
{
    return hitboxes.get(perGeometry.get(geo));
}
