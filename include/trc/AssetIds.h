#pragma once

#include <vkb/Image.h>

#include "Types.h"
#include "Geometry.h"
#include "Material.h"

namespace trc
{
    using AssetRegistryIdType = ui32;

    using GeometryID = TypesafeID<Geometry, AssetRegistryIdType>;
    using MaterialID = TypesafeID<Material, AssetRegistryIdType>;
    using TextureID  = TypesafeID<vkb::Image, AssetRegistryIdType>;
}
