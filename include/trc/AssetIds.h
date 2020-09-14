#pragma once

#include "Boilerplate.h"
#include "utils/TypesafeId.h"

namespace trc
{
    namespace internal
    {
        // I don't want to use the asset classes directly for the typesafe IDs.
        // It feels wrong.

        struct _GeometryAssetIdType {};
        struct _MaterialAssetIdType {};
        struct _TextureAssetIdType {};
    }

    using AssetRegistryIdType = ui32;
    using GeometryID = TypesafeID<internal::_GeometryAssetIdType, AssetRegistryIdType>;
    using MaterialID = TypesafeID<internal::_MaterialAssetIdType, AssetRegistryIdType>;
    using TextureID  = TypesafeID<internal::_TextureAssetIdType, AssetRegistryIdType>;
}
