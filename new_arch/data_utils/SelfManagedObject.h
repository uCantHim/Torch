#pragma once

#include <cstdint>
#include <memory>
#include <vector>
#include <optional>

#include "IndexMap.h"

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
     * @throws Error if the index is occupied
     */
    template<typename ...ConstructArgs>
    static auto create(size_t index, ConstructArgs&&... args) -> Derived&;

    static auto at(size_t index) -> Derived&;
    static auto find(size_t index) noexcept
        -> std::optional<std::reference_wrapper<Derived>>;

    static void destroy(size_t index);


    // ------------------
    // Non-static methods

    auto id() const noexcept -> ID;

private:
    using StoredType = Derived;

    static inline IndexMap<ID, StoredType> objects;
};
