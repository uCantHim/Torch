#pragma once

#include "Types.h"
#include "Geometry.h"
#include "Material.h"
#include "Texture.h"

namespace trc
{
    class AssetRegistry;

    using AssetIdNumericType = ui32;

    template<typename Derived>
    class AssetIdBase : public TypesafeID<Derived, AssetIdNumericType>
    {
    protected:
        constexpr AssetIdBase() = default;
        AssetIdBase(AssetIdNumericType id, AssetRegistry& ar)
            : TypesafeID<Derived, AssetIdNumericType>(id), ar(&ar)
        {}

    public:
        using ID = TypesafeID<Derived, AssetIdNumericType>;

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
    class GeometryID : public AssetIdBase<GeometryID>
    {
    public:
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
    class MaterialID : public AssetIdBase<MaterialID>
    {
    public:
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
    class TextureID : public AssetIdBase<TextureID>
    {
    public:
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
