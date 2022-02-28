#pragma once

#include "Types.h"
#include "AssetBase.h"

namespace trc
{
    class AssetInventory;

    struct _AssetIdTypeTag {};

    /**
     * @brief ID for basic metadata that is shared by all asset types
     */
    using AssetID = data::HardTypesafeID<_AssetIdTypeTag, AssetInventory, ui32>;

    /**
     * @brief ID for a specific asset type
     */
    template<AssetBaseType T>
    struct TypedAssetID
    {
        using LocalID = data::HardTypesafeID<T, AssetRegistryModule<T>, ui32>;

        /** ID that references metadata common to all assets */
        AssetID uniqueId;

        /** ID that references data in the asset type's specific asset registry */
        LocalID id;

        /**
         * Pointer to the asset type's specific registry module implementation
         * at which the asset is allocated.
         */
        AssetRegistryModule<T>* reg;

        TypedAssetID(AssetID base, LocalID type, AssetRegistryModule<T>& reg);

        auto getDeviceDataHandle()
        {
            return reg->getHandle(id);
        }
    };



    class GeometryRegistry;
    class TextureRegistry;
    class MaterialRegistry;
    struct GeometryData;
    struct TextureData;
    struct MaterialData;

    class _Geometry
    {
    public:
        using Registry = GeometryRegistry;
        using ImportData = GeometryData;
    };

    class _Texture
    {
    public:
        using Registry = TextureRegistry;
        using ImportData = TextureData;
    };

    class _Material
    {
    public:
        using Registry = MaterialRegistry;
        using ImportData = MaterialData;
    };



    using GeometryID = TypedAssetID<_Geometry>;
    using TextureID = TypedAssetID<_Texture>;
    using MaterialID = TypedAssetID<_Material>;



    // template<typename T>
    // concept AssetRegistryModuleType = requires (T a, AssetData<T> data, TypedAssetID<T> id)
    // {
    //     AssetBaseType<T>;
    //     { a.add(data) } -> std::same_as<TypedAssetID<T>>;
    //     { a.remove(id) };
    //     { a.getHandle(id) } -> std::same_as<AssetHandle<T>>;
    // };
} // namespace trc
