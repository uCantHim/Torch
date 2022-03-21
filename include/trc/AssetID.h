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
    public:
                           /* Hard */
        using LocalID = data::TypesafeID<T, /* AssetRegistryModule<T>, */ ui32>;

        TypedAssetID() = default;
        TypedAssetID(AssetID base, LocalID type, AssetManagerFwd& man)
            : uniqueId(base), id(type), manager(&man)
        {}

        operator bool() const {
            return manager != nullptr;
        }

        auto getAssetID() const -> AssetID;
        auto getDeviceID() const -> LocalID;

        auto get() const;
        auto getDeviceDataHandle() const;

    private:
        /** ID that references metadata common to all assets */
        AssetID uniqueId{ AssetID::NONE };

        /** ID that references data in the asset type's specific asset registry */
        LocalID id{ LocalID::NONE };

        AssetManagerFwd* manager{ nullptr };
    };



    template<AssetBaseType T>
    auto TypedAssetID<T>::getAssetID() const -> AssetID
    {
        return uniqueId;
    }

    template<AssetBaseType T>
    auto TypedAssetID<T>::getDeviceID() const -> LocalID
    {
        return id;
    }

    template<AssetBaseType T>
    auto TypedAssetID<T>::get() const
    {
        return getDeviceDataHandle();
    }

    template<AssetBaseType T>
    auto TypedAssetID<T>::getDeviceDataHandle() const
    {
        if (manager == nullptr)
        {
            throw std::runtime_error("[In TypedAssetID<>::getDeviceDataHandle]: Tried to get"
                                     " device data from an invalid asset ID.");
        }

        return manager->getModule<T>().getHandle(id);
    }
} // namespace trc
