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
    static_assert(std::is_integral_v<Key>, "Key must be an arithmetic type");
    static_assert(std::is_default_constructible_v<Value>,
                  "Value must be default constructible");

    auto operator[](Key key) noexcept -> Value&;
    auto operator[](Key key) const noexcept -> const Value&;

    auto at(Key key) -> Value&;
    auto find(Key key) -> std::optional<std::reference_wrapper<Value>>;

    auto remove(Key key) -> Value;

    void reserve(size_t size);
    auto getMaxIndex() const noexcept -> size_t;

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
