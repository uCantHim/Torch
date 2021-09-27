#pragma once

#include "Types.h"
#include "Geometry.h"
#include "Material.h"
#include "Texture.h"

namespace trc
{
    class AssetRegistry;

    using AssetIdNumericType = ui32;

    class AssetIdBase
    {
    protected:
        constexpr AssetIdBase() = default;
        AssetIdBase(AssetRegistry& ar) : ar(&ar) {}

    public:
        auto operator<=>(const AssetIdBase&) const = default;

        auto getAssetRegistry() -> AssetRegistry&
        {
            assert(ar != nullptr);
            return *ar;
        }

    protected:
        AssetRegistry* ar{ nullptr };
    };

    /**
     * @brief
     */
    class GeometryID : public TypesafeID<GeometryID, AssetIdNumericType>
                     , public AssetIdBase
    {
    public:
        using ID = TypesafeID<GeometryID, AssetIdNumericType>;

        /**
         * @brief
         */
        GeometryID() = default;

        auto get() const -> Geometry;

    protected:
        friend class AssetRegistry;
        GeometryID(AssetIdNumericType id, AssetRegistry& ar);
    };

    /**
     * @brief
     */
    class MaterialID : public TypesafeID<MaterialID, AssetIdNumericType>
                     , public AssetIdBase
    {
    public:
        using ID = TypesafeID<MaterialID, AssetIdNumericType>;
        /**
         * @brief
         */
        MaterialID() = default;

        auto get() const -> Material&;

    protected:
        friend class AssetRegistry;
        MaterialID(AssetIdNumericType id, AssetRegistry& ar);
    };

    /**
     * @brief
     */
    class TextureID : public TypesafeID<TextureID, AssetIdNumericType>
                     , public AssetIdBase
    {
    public:
        using ID = TypesafeID<TextureID, AssetIdNumericType>;

        /**
         * @brief
         */
        TextureID() = default;

        auto get() const -> Texture;

    protected:
        friend class AssetRegistry;
        TextureID(AssetIdNumericType id, AssetRegistry& ar);
    };
}
