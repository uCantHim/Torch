#pragma once

#include <cstdint>
#include <memory>
#include <vector>
#include <optional>

#include <vkb/VulkanBase.h>

#include "IndexMap.h"
#include "utils/TypesafeId.h"

namespace trc::data
{
    /**
     * @brief CRTP interface with a static object collection and unique IDs
     *
     * Useful for long-living, globally accessible objects with permanent IDs.
     *
     * Not useful for fast iteration or rapid reallocations.
     *
     * @tparam Derived The derived class
     */
    template<class Derived>
    class SelfManagedObject
    {
    public:
        using IDType = uint64_t;
        using ID = TypesafeID<Derived, IDType>;

        // ----------------
        // Static functions

        template<typename ...ConstructArgs>
        static auto createAtNextIndex(ConstructArgs&&... args)
            -> std::pair<ID, std::reference_wrapper<Derived>>;

        /**
         * @throws std::runtime_error if an object already exists at the index
         */
        template<typename ...ConstructArgs>
        static auto create(ID index, ConstructArgs&&... args) -> Derived&;

        template<typename Class, typename ...ConstructArgs>
        static auto create(ID index, ConstructArgs&&... args) -> Class&
            requires(std::is_polymorphic_v<Derived> && std::is_base_of_v<Derived, Class>);

        /**
         * @brief Like create(), but can overwrite existing objects
         */
        template<typename ...ConstructArgs>
        static auto emplace(ID index, ConstructArgs&&... args) -> Derived&;

        template<typename Class, typename ...ConstructArgs>
        static auto emplace(ID index, ConstructArgs&&... args) -> Class&
            requires(std::is_polymorphic_v<Derived> && std::is_base_of_v<Derived, Class>);

        /**
         * @throw std::out_of_range if no object at that index exists
         */
        static auto at(ID index) -> Derived&;

        /**
         * @throw std::out_of_range if no object at that index exists
         */
        static void destroy(ID index);

        // ------------------
        // Non-static methods

        auto id() const noexcept -> ID;

    private:
        ID myId;

        static inline IndexMap<IDType, std::unique_ptr<Derived>> objects;

        static inline vkb::StaticInit _init{
            []() {},
            []() { objects = {}; }
        };
    };


#include "SelfManagedObject.inl"

} // namespace trc::data
