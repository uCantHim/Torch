#pragma once

#include <trc_util/data/TypesafeId.h>

#include "Types.h"
#include "AssetBase.h"

namespace trc
{
    class AssetManager;

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

        /**
         * Pointer to the asset type's specific registry module implementation
         * at which the asset is allocated.
         */
        AssetRegistryModule<T>* reg{ nullptr };

        TypedAssetID() = default;
        TypedAssetID(AssetID base, LocalID type, AssetRegistryModule<T>& reg)
            : uniqueId(base), id(type), reg(&reg)
        {}

        auto get() const
        {
            return getDeviceDataHandle();
        }

        auto getDeviceDataHandle()
        {
            return reg->getHandle(id);
        }

        auto getDeviceDataHandle() const
        {
            return reg->getHandle(id);
        }
    };



    class GeometryRegistry;
    class TextureRegistry;
    class MaterialRegistry;
    struct GeometryData;
    struct TextureData;
    struct MaterialDeviceHandle;

    class Geometry
    {
    public:
        using Registry = GeometryRegistry;
        using ImportData = GeometryData;
    };

    class Texture
    {
    public:
        using Registry = TextureRegistry;
        using ImportData = TextureData;
    };

    class Material
    {
    public:
        using Registry = MaterialRegistry;
        using ImportData = MaterialDeviceHandle;
    };



    using GeometryID = TypedAssetID<Geometry>;
    using TextureID = TypedAssetID<Texture>;
    using MaterialID = TypedAssetID<Material>;
} // namespace trc
