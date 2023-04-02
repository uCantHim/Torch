#pragma once

#include <cassert>
#include <concepts>
#include <functional>
#include <optional>
#include <stdexcept>
#include <string>

#include <trc_util/data/SafeVector.h>
#include <trc_util/data/TypeMap.h>

#include "trc/assets/AssetBase.h"
#include "trc/assets/AssetTypeMap.h"

namespace trc
{
    /**
     * Base class for all asset traits.
     */
    class AssetTrait
    {
    public:
        virtual ~AssetTrait() noexcept = default;
    };

    template<typename T>
    concept AssetTraitT = std::derived_from<T, AssetTrait>;

    /**
     * @brief Storage for an asset type's traits
     *
     * Stores any number of trait implementations for any asset type (at most
     * one implementation for an [asset, trait] pair, but any number of pairs
     * with the same 'asset').
     */
    class TraitStorage
    {
    public:
        /**
         * @brief Register a trait implementation
         *
         * For an asset type A (e.g. trc::Geometry), register an implementation
         * of an asset trait type T.
         *
         * Overwrites any existing implementation of `T` for asset type `type`.
         *
         * @tparam T The asset trait type for which to register an
         *         implementation. This parameter must be specified explicitly.
         * @tparam Impl Deduced from function parameter `trait`.
         *
         * @param AssetType type The asset type for which to register the trait
         *        implementation.
         * @param u_ptr<Impl> trait The implementation of trait type `T`. Must
         *        not be nullptr.
         *
         * @throw std::invalid_argument if `trait` is nullptr.
         *
         * # Example
         * @code
         *  class Greeter : public trc::AssetTrait
         *  {
         *  public:
         *      virtual auto greet() -> std::string = 0;
         *  }
         *
         *  class GeometryGreeter : public Greeter
         *  {
         *  public:
         *      auto greet() -> std::string override {
         *          return "Hello! I am geometry :D";
         *      }
         *  };
         *
         *  const auto geoType = AssetType::make<trc::Geometry>();
         *  traitStorage.registerTrait<Greeter>(geoType, std::make_unique<GeometryGreeter>());
         *
         *  std::cout << traitStorage.getTrait<Greeter>(geoType).greet();
         * @endcode
         */
        template<AssetTraitT T, std::derived_from<T> Impl>
        void registerTrait(const AssetType& type, u_ptr<Impl> trait);

        /**
         * @brief Access a trait implementation for a specific asset type
         *
         * @tparam Trait The trait for which to retrieve an implementation. Must
         *         be specified explicitly.
         *
         * @param AssetType type The asset type for which to query an
         *        implementation of `T`.
         *
         * @return optional<Trait&> The implementation of trait `T`, if one is
         *         registered for for asset type `type`. nullopt otherwise.
         */
        template<AssetTraitT T>
        auto getTrait(const AssetType& type) noexcept
            -> std::optional<std::reference_wrapper<T>>;

    private:
        using TypeIndex = trc::data::TypeIndexAllocator<TraitStorage>;
        AssetTypeMap<util::SafeVector<u_ptr<AssetTrait>>> traits;
    };



    template<AssetTraitT T, std::derived_from<T> Impl>
    void TraitStorage::registerTrait(const AssetType& type, u_ptr<Impl> trait)
    {
        if (trait == nullptr) {
            throw std::invalid_argument("[In TraitStorage::registerTrait]: Argument `trait` must"
                                        " not be nullptr!");
        }

        auto [vec, _] = traits.try_emplace(type);
        vec.emplace(TypeIndex::get<T>(), std::move(trait));
    }

    template<AssetTraitT T>
    auto TraitStorage::getTrait(const AssetType& type) noexcept
        -> std::optional<std::reference_wrapper<T>>
    {
        auto [vec, _] = traits.try_emplace(type);

        const ui32 index = TypeIndex::get<T>();
        if (!vec.contains(index)) {
            return std::nullopt;
        }

        AssetTrait* trait = vec.at(index).get();
        assert(trait != nullptr);
        assert(dynamic_cast<T*>(trait) != nullptr);
        return *static_cast<T*>(trait);
    }
} // namespace trc
