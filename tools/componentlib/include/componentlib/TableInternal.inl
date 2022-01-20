#include "Table.h"



template<typename Component, TableKey Key>
inline auto Table<Component, Key>::_index_at_key(Key key) -> IndexType&
{
    return indices.at(static_cast<size_t>(key));
}

template<typename Component, TableKey Key>
inline auto Table<Component, Key>::_index_at_key(Key key) const -> const IndexType&
{
    return indices.at(static_cast<size_t>(key));
}

template<typename Component, TableKey Key>
inline auto Table<Component, Key>::_do_get(IndexType index) -> Component&
{
    if constexpr (stableStorage) {
        return *objects.at(index);
    }
    else {
        return objects.at(index);
    }
}

template<typename Component, TableKey Key>
inline auto Table<Component, Key>::_do_get(IndexType index) const -> const Component&
{
    if constexpr (stableStorage) {
        return *objects.at(index);
    }
    else {
        return objects.at(index);
    }
}

template<typename Component, TableKey Key>
template<typename ...Args>
inline auto Table<Component, Key>::_do_emplace(IndexType index, Args&&... args) -> Component&
{
    if constexpr (stableStorage)
    {
        return **objects.emplace(
            objects.begin() + index,
            new Component(std::forward<Args>(args)...)
        );
    }
    else
    {
        return *objects.emplace(
            objects.begin() + index,
            std::forward<Args>(args)...
        );
    }
}

template<typename Component, TableKey Key>
template<typename ...Args>
inline auto Table<Component, Key>::_do_emplace_back(Args&&... args)
    -> std::pair<IndexType, Component&>
{
    if constexpr (stableStorage)
    {
        return {
            objects.size(),
            *objects.emplace_back(new Component(std::forward<Args>(args)...))
        };
    }
    else
    {
        return {
            objects.size(),
            objects.emplace_back(std::forward<Args>(args)...)
        };
    }
}

template<typename Component, TableKey Key>
inline auto Table<Component, Key>::_do_erase_unsafe(Key key) -> Component
{
    auto indexIt = indices.begin() + static_cast<size_t>(key);

    // Unstably remove object
    std::swap(objects.at(*indexIt), objects.back());
    Component result = [this] {
        if constexpr (stableStorage) {
            return std::move(*objects.back());
        }
        else {
            return std::move(objects.back());
        }
    }();
    objects.pop_back();

    // Flag object as not existing and modify index that pointed at the moved object
    auto swappedObjectIndexIt = std::find(indices.begin(), indices.end(), objects.size());
    if (swappedObjectIndexIt != indices.end()) {
        *swappedObjectIndexIt = *indexIt;
    }
    *indexIt = NONE;

    return result;
}
