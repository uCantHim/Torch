#pragma once

#include <trc_util/data/TypesafeId.h>

#include "Types.h"
#include "AssetBase.h"
#include "AssetManagerInterface.h"

namespace trc
{
    struct _AssetIdTypeTag {};

    /**
     * @brief ID for basic metadata that is shared by all asset types
     */
    using AssetID = data::HardTypesafeID<_AssetIdTypeTag, AssetManager, ui32>;

    /**
     * @brief ID for a specific asset type
     */
    template<AssetBaseType T>
    struct TypedAssetID
    {
                           /* Hard */
        using LocalID = data::TypesafeID<T, /* AssetRegistryModule<T>, */ ui32>;

        /** ID that references metadata common to all assets */
        AssetID uniqueId{ AssetID::NONE };

        /** ID that references data in the asset type's specific asset registry */
        LocalID id{ LocalID::NONE };

        AssetManagerFwd* manager;

        TypedAssetID() = default;
        TypedAssetID(AssetID base, LocalID type, AssetManagerFwd& man)
            : uniqueId(base), id(type), manager(&man)
        {}

        auto get() const
        {
            return getDeviceDataHandle();
        }

        auto getDeviceDataHandle()
        {
            return manager->getModule<T>().getHandle(id);
        }

        auto getDeviceDataHandle() const
        {
            return manager->getModule<T>().getHandle(id);
        }
    };
} // namespace trc
