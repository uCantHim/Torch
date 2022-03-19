#pragma once

#include <concepts>

#include "AssetBase.h"

namespace trc
{
    template<AssetBaseType T>
    struct TypedAssetID;

    class AssetPath;
    class AssetManager;

    template<typename Derived>
    class AssetManagerInterface
    {
    public:
        AssetManagerInterface()
        {
            static_assert(std::derived_from<Derived, AssetManagerInterface<Derived>>,
                          "CRTP interface can only be instantiated with a derived type");
        }

        template<AssetBaseType T>
        auto getAsset(const AssetPath& path) const -> TypedAssetID<T>
        {
            return asDerived().getAsset(path);
        }

        template<AssetBaseType T>
        auto getModule() -> AssetRegistryModule<T>&
        {
            return asDerived().template getModule<T>();
        }

    private:
        constexpr auto asDerived() -> Derived&
        {
            return static_cast<Derived&>(*this);
        }
    };

    /** Forward-declaration of the AssetManager's interface instantiation */
    using AssetManagerFwd = AssetManagerInterface<AssetManager>;
} // namespace trc
