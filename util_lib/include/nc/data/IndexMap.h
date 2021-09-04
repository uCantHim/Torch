#pragma once

#include <vector>

namespace nc::data
{

/**
 * @brief A vector that allows map-like inserts and lookups
 */
template<typename Key, typename Value>
class IndexMap
{
public:
    IndexMap();

    /**
     * Contructs a new value if the index is not yet occupied.
     */
    auto operator[](Key key) noexcept -> Value&;

    /**
     * Throws if the index is not yet occupied.
     */
    auto operator[](Key key) const noexcept -> const Value&;

    auto at(Key key) -> Value&;

    template<typename ...Args>
    auto emplace(Key key, Args&&... args) -> Value&;

    auto remove(Key key) -> Value;

    auto size() const noexcept -> size_t;
    void reserve(size_t size);

private:
    std::vector<Value> values;
};

#include "IndexMap.inl"

} // namespace nc::data