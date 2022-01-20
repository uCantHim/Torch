#include "AssetIds.h"

#include "AssetRegistry.h"



trc::GeometryID::GeometryID(AssetIdNumericType id, AssetRegistry& ar)
    :
    AssetIdBase(id, ar)
{
}

auto trc::GeometryID::get() const -> Geometry
{
    return ar->get(*this);
}



trc::MaterialID::MaterialID(AssetIdNumericType id, AssetRegistry& ar)
    :
    AssetIdBase(id, ar)
{
}

auto trc::MaterialID::get() const -> Material&
{
    return ar->get(*this);
}



trc::TextureID::TextureID(AssetIdNumericType id, AssetRegistry& ar)
    :
    AssetIdBase(id, ar)
{
}

auto trc::TextureID::get() const -> Texture
{
    return ar->get(*this);
}
