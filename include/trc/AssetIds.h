#pragma once

#include "Types.h"
#include "Geometry.h"
#include "Material.h"
#include "Texture.h"

namespace trc
{
    class AssetRegistry;

    using AssetIdType = ui32;

    template<typename Derived>
    class AssetID : public TypesafeID<Derived, AssetIdType>
    {
    private:
        friend class AssetRegistry;

        AssetID(ui32 id, AssetRegistry& ar);

    public:
        using ID = TypesafeID<Derived, AssetIdType>;

        constexpr AssetID() = default;

        inline auto operator<=>(const AssetID<Derived>&) const = default;

        inline auto operator*();

        inline auto id() const -> AssetIdType;
        inline auto get();
        inline auto getAssetRegistry() -> AssetRegistry&;

    private:
        AssetRegistry* ar{ nullptr };
    };

    using GeometryID = AssetID<Geometry>;
    using MaterialID = AssetID<Material>;
    using TextureID  = AssetID<Texture>;
}
