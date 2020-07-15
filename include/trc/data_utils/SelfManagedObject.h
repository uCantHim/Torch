#pragma once

#include <cstdint>
#include <memory>
#include <vector>
#include <optional>

#include "IndexMap.h"

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
        using ID = uint64_t;

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
        static inline IndexMap<ID, std::unique_ptr<Derived>> objects;

        ID myId;
    };


#include "SelfManagedObject.inl"

} // namespace trc::data
