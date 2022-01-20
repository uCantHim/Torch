#include "TableIterators.h"



// ------------------------ //
//      Value Iterator      //
// ------------------------ //

template<typename TableType>
inline TableValueIterator<TableType>::TableValueIterator(VectorIterator _it)
    :
    it(_it)
{
}

template<typename TableType>
inline auto TableValueIterator<TableType>::operator*() -> value_type&
    requires (!std::is_const_v<TableType>)
{
    return getCurrentRef();
}

template<typename TableType>
inline auto TableValueIterator<TableType>::operator*() const -> const value_type&
{
    return getCurrentRef();
}

template<typename TableType>
inline auto TableValueIterator<TableType>::operator->() -> value_type*
    requires (!std::is_const_v<TableType>)
{
    return &getCurrentRef();
}

template<typename TableType>
inline auto TableValueIterator<TableType>::operator->() const -> const value_type*
{
    return &getCurrentRef();
}

template<typename TableType>
inline auto TableValueIterator<TableType>::operator++() -> TableValueIterator&
{
    ++it;
    return *this;
}

template<typename TableType>
inline auto TableValueIterator<TableType>::operator++(int) -> TableValueIterator
{
    auto result = *this;
    ++*this;
    return result;
}

template<typename TableType>
inline auto TableValueIterator<TableType>::operator--() -> TableValueIterator&
{
    --it;
    return *this;
}

template<typename TableType>
inline auto TableValueIterator<TableType>::operator--(int) -> TableValueIterator
{
    auto result = *this;
    --*this;
    return result;
}

template<typename TableType>
inline bool TableValueIterator<TableType>::operator==(const TableValueIterator& other) const
{
    return other.it == it;
}

template<typename TableType>
inline bool TableValueIterator<TableType>::operator!=(const TableValueIterator& other) const
{
    return other.it != it;
}

template<typename TableType>
inline void TableValueIterator<TableType>::swap(TableValueIterator& other)
{
    std::swap(other.it, it);
}

template<typename TableType>
inline auto TableValueIterator<TableType>::getCurrentRef() -> value_type&
{
    if constexpr (TableType::stableStorage) {
        return **it;
    }
    else {
        return *it;
    }
}

template<typename TableType>
inline auto TableValueIterator<TableType>::getCurrentRef() const -> const value_type&
{
    if constexpr (TableType::stableStorage) {
        return **it;
    }
    else {
        return *it;
    }
}



// ---------------------- //
//      Key Iterator      //
// ---------------------- //

template<typename TableType>
inline TableKeyIterator<TableType>::TableKeyIterator(
    typename std::vector<size_t>::const_iterator _it,
    TableType& table)
    :
    it(_it),
    table(&table),
    currentKey(it - table.indices.begin())
{
    while (it < table.indices.end() && *it == TableType::NONE)
    {
        ++it;
        incrementCurrentKey();
    }
}

template<typename TableType>
inline auto TableKeyIterator<TableType>::operator*() -> key_type&
{
    return currentKey;
}

template<typename TableType>
inline auto TableKeyIterator<TableType>::operator*() const -> const key_type&
{
    return currentKey;
}

template<typename TableType>
inline auto TableKeyIterator<TableType>::operator->() -> key_type*
{
    return &currentKey;
}

template<typename TableType>
inline auto TableKeyIterator<TableType>::operator->() const -> const key_type*
{
    return &currentKey;
}

template<typename TableType>
inline auto TableKeyIterator<TableType>::operator++() -> TableKeyIterator&
{
    do {
        ++it;
        incrementCurrentKey();
    } while (it < table->indices.end() && *it == TableType::NONE);
    return *this;
}

template<typename TableType>
inline auto TableKeyIterator<TableType>::operator++(int) -> TableKeyIterator
{
    auto result = *this;
    ++*this;
    return result;
}

template<typename TableType>
inline auto TableKeyIterator<TableType>::operator--() -> TableKeyIterator&
{
    while (it >= table->indices.begin() && *it == TableType::NONE)
    {
        --it;
        decrementCurrentKey();
    }
    return *this;
}

template<typename TableType>
inline auto TableKeyIterator<TableType>::operator--(int) -> TableKeyIterator
{
    auto result = *this;
    --*this;
    return result;
}

template<typename TableType>
inline bool TableKeyIterator<TableType>::operator==(const TableKeyIterator& other) const
{
    return other.it == it;
}

template<typename TableType>
inline bool TableKeyIterator<TableType>::operator!=(const TableKeyIterator& other) const
{
    return other.it != it;
}

template<typename TableType>
inline void TableKeyIterator<TableType>::swap(TableKeyIterator& other)
{
    std::swap(other.it, it);
    std::swap(other.table, table);
    std::swap(other.currentKey, currentKey);
}

template<typename TableType>
inline auto TableKeyIterator<TableType>::queryValue() -> conditionally_const_value_type&
{
    return table->get(currentKey);
}

template<typename TableType>
inline void TableKeyIterator<TableType>::incrementCurrentKey()
{
    if constexpr (requires { ++currentKey; })
    {
        ++currentKey;
    }
    else {
        currentKey = key_type(static_cast<size_t>(currentKey) + 1);
    }
}

template<typename TableType>
inline void TableKeyIterator<TableType>::decrementCurrentKey()
{
    if constexpr (requires { --currentKey; })
    {
        --currentKey;
    }
    else {
        currentKey = key_type(static_cast<size_t>(currentKey) - 1);
    }
}



// ----------------------- //
//      Pair Iterator      //
// ----------------------- //

template<typename TableType>
inline TablePairIterator<TableType>::TablePairIterator(TableKeyIterator<TableType> it)
    :
    keyIt(it)
{
}

template<typename TableType>
inline auto TablePairIterator<TableType>::operator*() -> KeyValuePair
{
    return { *keyIt, keyIt.queryValue() };
}

template<typename TableType>
inline auto TablePairIterator<TableType>::operator*() const -> const KeyValuePair
{
    return { *keyIt, keyIt.queryValue() };
}

template<typename TableType>
inline auto TablePairIterator<TableType>::operator++() -> TablePairIterator&
{
    ++keyIt;
    return *this;
}

template<typename TableType>
inline auto TablePairIterator<TableType>::operator++(int) -> TablePairIterator
{
    auto result = *this;
    ++*this;
    return result;
}

template<typename TableType>
inline auto TablePairIterator<TableType>::operator--() -> TablePairIterator&
{
    --keyIt;
    return *this;
}

template<typename TableType>
inline auto TablePairIterator<TableType>::operator--(int) -> TablePairIterator
{
    auto result = *this;
    --*this;
    return result;
}

template<typename TableType>
inline bool TablePairIterator<TableType>::operator==(const TablePairIterator& other) const
{
    return other.keyIt == keyIt;
}

template<typename TableType>
inline bool TablePairIterator<TableType>::operator!=(const TablePairIterator& other) const
{
    return other.keyIt != keyIt;
}

template<typename TableType>
inline void TablePairIterator<TableType>::swap(TablePairIterator& other)
{
    std::swap(other.keyIt, keyIt);
}
