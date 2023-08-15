#include "Table.h"



// -------------------------------- //
//      Table implementations       //
// -------------------------------- //

template<typename T, TableKey Key>
inline auto Table<T, Key>::size() const -> size_t
{
    return objects.size();
}

template<typename T, TableKey Key>
inline auto Table<T, Key>::data() -> pointer
{
    return objects.data();
}

template<typename T, TableKey Key>
inline auto Table<T, Key>::data() const -> const_pointer
{
    return objects.data();
}

template<typename T, TableKey Key>
inline bool Table<T, Key>::has(Key key) const
{
    return indices.size() > static_cast<size_t>(key) && indices.at(static_cast<size_t>(key)) != NONE;
}

template<typename T, TableKey Key>
inline auto Table<T, Key>::get(Key key) -> reference
{
    return _do_get(_index_at_key(key));
}

template<typename T, TableKey Key>
inline auto Table<T, Key>::get(Key key) const -> const_reference
{
    return _do_get(_index_at_key(key));
}

template<typename T, TableKey Key>
inline auto Table<T, Key>::try_get(Key key) -> std::optional<pointer>
{
    if (!has(key)) {
        return std::nullopt;
    }

    return &get(key);
}

template<typename T, TableKey Key>
inline auto Table<T, Key>::try_get(Key key) const -> std::optional<const_pointer>
{
    if (!has(key)) {
        return std::nullopt;
    }

    return &get(key);
}

template<typename T, TableKey Key>
inline auto Table<T, Key>::get_m(Key key) -> trc::Maybe<reference>
{
    if (!has(key)) {
        return {};
    }
    return get(key);
}

template<typename T, TableKey Key>
inline auto Table<T, Key>::get_m(Key key) const -> trc::Maybe<const_reference>
{
    if (!has(key)) {
        return {};
    }
    return get(key);
}

template<typename T, TableKey Key>
template<typename ...Args>
    requires std::constructible_from<T, Args&&...>
inline auto Table<T, Key>::emplace(Key key, Args&&... args) -> reference
{
    auto [result, success] = try_emplace(key, std::forward<Args>(args)...);
    if (!success) {
        return _do_emplace(_index_at_key(key), std::forward<Args>(args)...);
    }

    return result;
}

template<typename T, TableKey Key>
template<typename ...Args>
    requires std::constructible_from<T, Args&&...>
inline auto Table<T, Key>::try_emplace(Key key, Args&&... args) -> std::pair<reference, bool>
{
    if (static_cast<size_t>(key) >= indices.size()) {
        indices.resize(static_cast<size_t>(key) + 1, NONE);
    }

    auto& index = _index_at_key(key);
    if (index != NONE) {
        return { _do_get(index), false };
    }

    auto [newIndex, obj] = _do_emplace_back(std::forward<Args>(args)...);
    index = newIndex;
    return { obj, true };
}

template<typename T, TableKey Key>
inline auto Table<T, Key>::erase(Key key) -> value_type
{
    if (!has(key)) {
        throw std::out_of_range("In Table<>::erase: No object exists at key "
                                + std::to_string(static_cast<size_t>(key)) + "");
    }

    return _do_erase_unsafe(key);
}

template<typename T, TableKey Key>
inline auto Table<T, Key>::try_erase(Key key) noexcept -> std::optional<value_type>
{
    if (!has(key)) {
        return std::nullopt;
    }

    return _do_erase_unsafe(key);
}

template<typename T, TableKey Key>
inline auto Table<T, Key>::erase_m(Key key) noexcept -> trc::Maybe<value_type>
{
    if (!has(key)) {
        return {};
    }

    return _do_erase_unsafe(key);
}

template<typename T, TableKey Key>
inline void Table<T, Key>::clear() noexcept
{
    objects.clear();
    indices.clear();
}

template<typename T, TableKey Key>
inline void Table<T, Key>::reserve(size_t minSize)
{
    reserve(minSize, minSize);
}

template<typename T, TableKey Key>
inline void Table<T, Key>::reserve(size_t minSizeElems, size_t minSizeKeys)
{
    objects.reserve(minSizeElems);
    indices.reserve(minSizeKeys);
}

template<typename T, TableKey Key>
inline auto Table<T, Key>::begin() -> ValueIterator
{
    return ValueIterator(objects.begin());
}

template<typename T, TableKey Key>
inline auto Table<T, Key>::begin() const -> ConstValueIterator
{
    return ConstValueIterator(objects.begin());
}

template<typename T, TableKey Key>
inline auto Table<T, Key>::end() -> ValueIterator
{
    return ValueIterator(objects.end());
}

template<typename T, TableKey Key>
inline auto Table<T, Key>::end() const -> ConstValueIterator
{
    return ConstValueIterator(objects.end());
}

template<typename T, TableKey Key>
inline auto Table<T, Key>::keyBegin() -> KeyIterator
{
    return KeyIterator(indices.begin(), *this);
}

template<typename T, TableKey Key>
inline auto Table<T, Key>::keyBegin() const -> ConstKeyIterator
{
    return ConstKeyIterator(indices.begin(), *this);
}

template<typename T, TableKey Key>
inline auto Table<T, Key>::keyEnd() -> KeyIterator
{
    return KeyIterator(indices.end(), *this);
}

template<typename T, TableKey Key>
inline auto Table<T, Key>::keyEnd() const -> ConstKeyIterator
{
    return ConstKeyIterator(indices.end(), *this);
}

template<typename T, TableKey Key>
inline auto Table<T, Key>::values() -> IteratorRange<ValueIterator>
{
    return { begin(), end() };
}

template<typename T, TableKey Key>
inline auto Table<T, Key>::values() const -> IteratorRange<ConstValueIterator>
{
    return { begin(), end() };
}

template<typename T, TableKey Key>
inline auto Table<T, Key>::keys() -> IteratorRange<KeyIterator>
{
    return { keyBegin(), keyEnd() };
}

template<typename T, TableKey Key>
inline auto Table<T, Key>::keys() const -> IteratorRange<ConstKeyIterator>
{
    return { keyBegin(), keyEnd() };
}

template<typename T, TableKey Key>
inline auto Table<T, Key>::items() -> IteratorRange<PairIterator>
{
    return { PairIterator(keyBegin()), PairIterator(keyEnd()) };
}

template<typename T, TableKey Key>
inline auto Table<T, Key>::items() const -> IteratorRange<ConstPairIterator>
{
    return { ConstPairIterator(keyBegin()), ConstPairIterator(keyEnd()) };
}
