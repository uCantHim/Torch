#pragma once

#include <trc_util/data/TypesafeId.h>

#include "trc/Types.h"
#include "trc/assets/AssetBase.h"
#include "trc/assets/AssetManagerInterface.h"

namespace trc
{
    struct AssetMetaData;

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

        /**
         * Returns `*this != NONE`.
         *
         * @return bool True if the asset ID is valid, false otherwise
         */
        operator bool() const;


        //////////////////////////////
        //  Comparison with NONE value

        using NoneType = typename LocalID::NoneType;
        static constexpr NoneType NONE{};

        bool operator==(const NoneType&) const;
        bool operator!=(const NoneType&) const;

        auto operator=(const NoneType&) -> TypedAssetID<T>&;


        ////////////////
        //  Asset access

        /** @return AssetID Global ID for type-agnostic asset data */
        auto getAssetID() const -> AssetID;
        /** @return AssetID Local ID for type-specific asset data */
        auto getDeviceID() const -> LocalID;

        auto get() const;
        auto getDeviceDataHandle() const;
        auto getMetaData() const -> const AssetMetaData&;

        auto getAssetManager() -> AssetManagerFwd&;
        auto getModule() -> AssetRegistryModule<T>&;

    private:
        /** ID that references metadata common to all assets */
        AssetID uniqueId{ AssetID::NONE };

        /** ID that references data in the asset type's specific asset registry */
        LocalID id{ LocalID::NONE };

        AssetManagerFwd* manager{ nullptr };
    };



    template<AssetBaseType T>
    TypedAssetID<T>::operator bool() const
    {
        // If one is NONE, all others must also be NONE
        // If that's not the case, something really bad probably happened
        assert((uniqueId == AssetID::NONE && id == LocalID::NONE && manager == nullptr)
            || (uniqueId != AssetID::NONE && id != LocalID::NONE && manager != nullptr));

        return manager != nullptr;
    }

    template<AssetBaseType T>
    bool TypedAssetID<T>::operator==(const NoneType&) const
    {
        return !*this;
    }

    template<AssetBaseType T>
    bool TypedAssetID<T>::operator!=(const NoneType&) const
    {
        return !(*this == NONE);
    }

    template<AssetBaseType T>
    auto TypedAssetID<T>::operator=(const NoneType&) -> TypedAssetID<T>&
    {
        uniqueId = AssetID::NONE;
        id = LocalID::NONE;
        manager = nullptr;

        return *this;
    }

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

    template<AssetBaseType T>
    auto TypedAssetID<T>::getMetaData() const -> const AssetMetaData&
    {
        return manager->getMetaData(*this);
    }

    template<AssetBaseType T>
    auto TypedAssetID<T>::getAssetManager() -> AssetManagerFwd&
    {
        assert(manager != nullptr);
        return *manager;
    }

    template<AssetBaseType T>
    auto TypedAssetID<T>::getModule() -> AssetRegistryModule<T>&
    {
        assert(manager != nullptr);
        return manager->getModule<T>();
    }
} // namespace trc
