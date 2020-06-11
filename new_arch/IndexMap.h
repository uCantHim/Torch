#pragma once

#include <vector>
#include <type_traits>
#include <optional>

/**
 * @brief A vector that allows map-like inserts and lookups
 *
 * Use only for either small maximum key values or densely populated maps.
 */
template<typename Key, typename Value>
class IndexMap
{
public:
    IndexMap();

    auto operator[](Key key) noexcept -> Value&;
    auto operator[](Key key) const noexcept -> const Value&;

    auto at(Key key) -> Value&;

    auto remove(Key key) -> Value;

    auto size() const noexcept -> size_t;
    void reserve(size_t size);

    /**
     * TODO
     *
     * Iterators that skip unpopulated entries:
     * auto begin();
     * auto end();
     *
     * auto emplace();
     */

private:
    std::vector<Value> values;
};


#include "IndexMap.inl"
