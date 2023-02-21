#pragma once

#include <concepts>
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
     */
    class TraitStorage
    {
    public:
        template<AssetTraitT T, std::derived_from<T> Impl>
        void registerTrait(const AssetType& type, u_ptr<Impl> trait);

        template<AssetTraitT Trait>
        auto getTrait(const AssetType& type) -> Trait&;

    private:
        using TypeIndex = trc::data::TypeIndexAllocator<TraitStorage>;

        AssetTypeMap<util::SafeVector<u_ptr<AssetTrait>>> traits;
    };



    template<AssetTraitT T, std::derived_from<T> Impl>
    void TraitStorage::registerTrait(const AssetType& type, u_ptr<Impl> trait)
    {
        auto [vec, _] = traits.try_emplace(type);
        vec.emplace(TypeIndex::get<T>(), std::move(trait));
    }

    template<AssetTraitT Trait>
    auto TraitStorage::getTrait(const AssetType& type) -> Trait&
    {
        auto [vec, _] = traits.try_emplace(type);

        const ui32 index = TypeIndex::get<Trait>();
        if (!vec.contains(index)) {
            throw std::out_of_range("Asset trait [" + std::string(typeid(Trait).name())
                                    + "] is not registered.");
        }

        return *static_cast<Trait*>(vec.at(index).get());
    }
} // namespace trc
