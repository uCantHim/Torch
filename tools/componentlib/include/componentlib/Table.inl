#include "Table.h"



// -------------------------------- //
//      Table implementations       //
// -------------------------------- //

template<typename Component, TableKey Key>
inline auto Table<Component, Key>::size() const -> size_t
{
    return objects.size();
}

template<typename Component, TableKey Key>
inline bool Table<Component, Key>::has(Key key) const
{
    return indices.size() > key && indices.at(static_cast<size_t>(key)) != NONE;
}

template<typename Component, TableKey Key>
inline auto Table<Component, Key>::get(Key key) -> Component&
{
    return _do_get(_index_at_key(key));
}

template<typename Component, TableKey Key>
inline auto Table<Component, Key>::get(Key key) const -> const Component&
{
    return _do_get(_index_at_key(key));
}

template<typename Component, TableKey Key>
inline auto Table<Component, Key>::try_get(Key key) -> std::optional<Component*>
{
    if (!has(key)) {
        return std::nullopt;
    }

    return &get(key);
}

template<typename Component, TableKey Key>
inline auto Table<Component, Key>::try_get(Key key) const -> std::optional<const Component*>
{
    if (!has(key)) {
        return std::nullopt;
    }

    return &get(key);
}

template<typename Component, TableKey Key>
inline auto Table<Component, Key>::get_m(Key key) -> trc::Maybe<Component&>
{
    if (!has(key)) {
        return {};
    }
    return get(key);
}

template<typename Component, TableKey Key>
inline auto Table<Component, Key>::get_m(Key key) const -> trc::Maybe<const Component&>
{
    if (!has(key)) {
        return {};
    }
    return get(key);
}

template<typename Component, TableKey Key>
template<typename ...Args>
    requires std::constructible_from<Component, Args&&...>
inline auto Table<Component, Key>::emplace(Key key, Args&&... args) -> Component&
{
    auto [result, success] = try_emplace(key, std::forward<Args>(args)...);
    if (!success) {
        return _do_emplace(_index_at_key(key), std::forward<Args>(args)...);
    }

    return result;
}

template<typename Component, TableKey Key>
template<typename ...Args>
    requires std::constructible_from<Component, Args&&...>
inline auto Table<Component, Key>::try_emplace(Key key, Args&&... args) -> std::pair<Component&, bool>
{
    if (key >= indices.size()) {
        indices.resize(key + 1, NONE);
    }

    auto& index = _index_at_key(key);
    if (index != NONE) {
        return { _do_get(index), false };
    }

    auto [newIndex, obj] = _do_emplace_back(std::forward<Args>(args)...);
    index = newIndex;
    return { obj, true };
}

template<typename Component, TableKey Key>
inline auto Table<Component, Key>::erase(Key key) -> Component
{
    if (!has(key)) {
        throw std::out_of_range("In Table<>::erase: No object exists at key "
                                + std::to_string(static_cast<size_t>(key)) + "");
    }

    return _do_erase_unsafe(key);
}

template<typename Component, TableKey Key>
inline auto Table<Component, Key>::try_erase(Key key) noexcept -> std::optional<Component>
{
    if (!has(key)) {
        return std::nullopt;
    }

    return _do_erase_unsafe(key);
}

template<typename Component, TableKey Key>
inline auto Table<Component, Key>::erase_m(Key key) noexcept -> trc::Maybe<Component>
{
    if (!has(key)) {
        return {};
    }

    return _do_erase_unsafe(key);
}

template<typename Component, TableKey Key>
inline void Table<Component, Key>::clear() noexcept
{
    objects.clear();
    indices.clear();
}

template<typename Component, TableKey Key>
inline auto Table<Component, Key>::begin() -> ValueIterator
{
    return ValueIterator(objects.begin());
}

template<typename Component, TableKey Key>
inline auto Table<Component, Key>::begin() const -> ConstValueIterator
{
    return ConstValueIterator(objects.begin());
}

template<typename Component, TableKey Key>
inline auto Table<Component, Key>::end() -> ValueIterator
{
    return ValueIterator(objects.end());
}

template<typename Component, TableKey Key>
inline auto Table<Component, Key>::end() const -> ConstValueIterator
{
    return ConstValueIterator(objects.end());
}

template<typename Component, TableKey Key>
inline auto Table<Component, Key>::keyBegin() -> KeyIterator
{
    return KeyIterator(indices.begin(), *this);
}

template<typename Component, TableKey Key>
inline auto Table<Component, Key>::keyBegin() const -> ConstKeyIterator
{
    return ConstKeyIterator(indices.begin(), *this);
}

template<typename Component, TableKey Key>
inline auto Table<Component, Key>::keyEnd() -> KeyIterator
{
    return KeyIterator(indices.end(), *this);
}

template<typename Component, TableKey Key>
inline auto Table<Component, Key>::keyEnd() const -> ConstKeyIterator
{
    return ConstKeyIterator(indices.end(), *this);
}

template<typename Component, TableKey Key>
inline auto Table<Component, Key>::values() -> IteratorRange<ValueIterator>
{
    return { begin(), end() };
}

template<typename Component, TableKey Key>
inline auto Table<Component, Key>::values() const -> IteratorRange<ConstValueIterator>
{
    return { begin(), end() };
}

template<typename Component, TableKey Key>
inline auto Table<Component, Key>::keys() -> IteratorRange<KeyIterator>
{
    return { keyBegin(), keyEnd() };
}

template<typename Component, TableKey Key>
inline auto Table<Component, Key>::keys() const -> IteratorRange<ConstKeyIterator>
{
    return { keyBegin(), keyEnd() };
}

template<typename Component, TableKey Key>
inline auto Table<Component, Key>::items() -> IteratorRange<PairIterator>
{
    return { PairIterator(keyBegin()), PairIterator(keyEnd()) };
}

template<typename Component, TableKey Key>
inline auto Table<Component, Key>::items() const -> IteratorRange<ConstPairIterator>
{
    return { ConstPairIterator(keyBegin()), ConstPairIterator(keyEnd()) };
}
