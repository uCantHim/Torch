#include "IndexMap.h"

#include <cassert>



template<typename Key, typename Value>
inline IndexMap<Key, Value>::IndexMap()
{
    static_assert(std::is_integral_v<Key>, "Key must be an arithmetic type");
    static_assert(std::is_default_constructible_v<Value>,
                  "Value must be default constructible");
}

template<typename Key, typename Value>
inline auto IndexMap<Key, Value>::operator[](Key key) noexcept -> Value&
{
    assert(key >= 0);

    if (key >= values.size()) {
        values.resize(key + 1);
    }

    return values.at(key);
}

template<typename Key, typename Value>
inline auto IndexMap<Key, Value>::operator[](Key key) const noexcept -> const Value&
{
    assert(key < values.size());
    assert(key >= 0);

    return values.at(key);
}

template<typename Key, typename Value>
inline auto IndexMap<Key, Value>::at(Key key) -> Value&
{
    assert(key < values.size());
    assert(key >= 0);

    return values.at(key);
}

template<typename Key, typename Value>
template<typename ...Args>
inline auto IndexMap<Key, Value>::emplace(Key key, Args&&... args) -> Value&
{
    assert(key >= 0);

    if (key >= values.size()) {
        values.resize(key + 1);
    }

    values[key] = Value(std::forward<Args>(args)...);
    return values[key];
}

template<typename Key, typename Value>
inline auto IndexMap<Key, Value>::remove(Key key) -> Value
{
    assert(key < values.size());
    assert(key >= 0);

    auto currentValue = std::move(values.at(key));
    values.at(key) = {};

    return currentValue;
}

template<typename Key, typename Value>
inline auto IndexMap<Key, Value>::size() const noexcept -> size_t
{
    return values.size();
}

template<typename Key, typename Value>
inline void IndexMap<Key, Value>::reserve(size_t size)
{
    values.reserve(size);
}
