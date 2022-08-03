#include "IndexMap.h"



template<typename Key, typename Value>
inline trc::data::IndexMap<Key, Value>::IndexMap()
{
    static_assert(std::is_convertible_v<Key, size_t>,
                  "Key of IndexMap must be convertible to size_t!");
    static_assert(std::is_default_constructible_v<Value>,
                  "Value must be default constructible");
}

template<typename Key, typename Value>
inline auto trc::data::IndexMap<Key, Value>::operator[](Key key) -> Value&
{
    if (key >= static_cast<Key>(values.size())) {
        values.resize(key + 1);
    }

    return values.at(key);
}

template<typename Key, typename Value>
inline auto trc::data::IndexMap<Key, Value>::operator[](Key key) const -> const Value&
{
    return values.at(key);
}

template<typename Key, typename Value>
inline auto trc::data::IndexMap<Key, Value>::at(Key key) -> Value&
{
    return values.at(key);
}

template<typename Key, typename Value>
inline auto trc::data::IndexMap<Key, Value>::at(Key key) const -> const Value&
{
    return values.at(key);
}

template<typename Key, typename Value>
template<typename ...Args>
inline auto trc::data::IndexMap<Key, Value>::emplace(Key key, Args&&... args) -> Value&
{
    if (key >= values.size()) {
        values.resize(key + 1);
    }

    values[key] = Value(std::forward<Args>(args)...);
    return values[key];
}

template<typename Key, typename Value>
inline auto trc::data::IndexMap<Key, Value>::remove(Key key) -> Value
{
    auto currentValue = std::move(values.at(key));
    values.at(key) = {};

    return currentValue;
}

template<typename Key, typename Value>
inline auto trc::data::IndexMap<Key, Value>::size() const noexcept -> size_t
{
    return values.size();
}

template<typename Key, typename Value>
inline void trc::data::IndexMap<Key, Value>::reserve(size_t size)
{
    values.reserve(size);
}
