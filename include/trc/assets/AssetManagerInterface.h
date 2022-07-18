#pragma once

#include <concepts>

#include "Types.h"
#include "AssetBase.h"
#include "AssetSource.h"  // This is not strictly necessary, a forward declaration works

namespace trc
{
    class AssetPath;

    template<AssetBaseType T>
    struct TypedAssetID;

    class AssetManager;
    struct AssetMetaData;

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
        auto create(const AssetData<T>& data) -> TypedAssetID<T>
        {
            return asDerived().template create<T>(data);
        }

        template<AssetBaseType T>
        auto create(u_ptr<AssetSource<T>> source) -> TypedAssetID<T>
        {
            return asDerived().template create<T>(std::move(source));
        }

        template<AssetBaseType T>
        inline auto create(const AssetPath& path) -> TypedAssetID<T>
        {
            return asDerived().template create<T>(path);
        }

        template<AssetBaseType T>
        inline void destroy(TypedAssetID<T> id)
        {
            asDerived().template destroy<T>(id);
        }

        inline bool exists(const AssetPath& path) const
        {
            return asDerived().exists(path);
        }

        template<AssetBaseType T>
        inline auto get(const AssetPath& path) const -> TypedAssetID<T>
        {
            return asDerived().template get<T>(path);
        }

        template<AssetBaseType T>
        auto getMetaData(TypedAssetID<T> id) const -> const AssetMetaData&
        {
            return asDerived().template getMetaData(id);
        }

        template<AssetBaseType T>
        inline auto getModule() -> AssetRegistryModule<T>&
        {
            return asDerived().template getModule<T>();
        }

    private:
        constexpr auto asDerived() -> Derived& {
            return static_cast<Derived&>(*this);
        }

        constexpr auto asDerived() const -> const Derived& {
            return static_cast<const Derived&>(*this);
        }
    };

    /** Forward-declaration of the AssetManager's interface instantiation */
    using AssetManagerFwd = AssetManagerInterface<AssetManager>;
} // namespace trc
