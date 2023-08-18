#pragma once

#include <cassert>
#include <concepts>
#include <iterator>
#include <type_traits>
#include <vector>

namespace componentlib
{

/**
 * @brief Iterator over keys in a table
 */
template<typename TableType>
struct IndirectTableValueIterator
{
private:
    using VectorIterator = std::conditional_t<
        std::is_const_v<TableType>,
        typename std::vector<typename TableType::value_type>::const_iterator,
        typename std::vector<typename TableType::value_type>::iterator
    >;

public:
    // Bidirectional for now. LegacyRandomAccessIterator is much more complex
    using iterator_category = std::bidirectional_iterator_tag;

    using value_type = typename TableType::value_type;
    using conditionally_const_value_type = std::conditional_t<
        std::is_const_v<TableType>,
        std::add_const_t<value_type>,
        value_type
    >;
    using reference = conditionally_const_value_type&;
    using pointer = conditionally_const_value_type*;
    using key_type = typename TableType::key_type;

    using difference_type = size_t;

    IndirectTableValueIterator() = default;
    explicit
    IndirectTableValueIterator(VectorIterator it);

    IndirectTableValueIterator(const IndirectTableValueIterator&) = default;
    auto operator=(const IndirectTableValueIterator&) -> IndirectTableValueIterator& = default;
    IndirectTableValueIterator(IndirectTableValueIterator&&) noexcept = default;
    auto operator=(IndirectTableValueIterator&&) noexcept -> IndirectTableValueIterator& = default;

    ~IndirectTableValueIterator() = default;

    // Prefix
    auto operator++() -> IndirectTableValueIterator&;
    // Postfix
    auto operator++(int) -> IndirectTableValueIterator;

    // Prefix
    auto operator--() -> IndirectTableValueIterator&;
    // Postfix
    auto operator--(int) -> IndirectTableValueIterator;

    auto operator*() -> reference;
    auto operator->() -> pointer;

    bool operator==(const IndirectTableValueIterator& a) const;
    bool operator!=(const IndirectTableValueIterator& a) const;

    void swap(IndirectTableValueIterator& other);

private:
    VectorIterator it;
};

/**
 * @brief Iterator over keys in a table
 */
template<typename TableType>
struct IndirectTableKeyIterator
{
    // Bidirectional for now. LegacyRandomAccessIterator is much more complex
    using iterator_category = std::bidirectional_iterator_tag;

    using value_type = typename TableType::key_type;
    using key_type = typename TableType::key_type;
    using reference = const key_type&;
    using pointer = const key_type*;

    using difference_type = size_t;

    IndirectTableKeyIterator() = default;
    IndirectTableKeyIterator(typename std::vector<size_t>::const_iterator it, TableType& table);

    IndirectTableKeyIterator(const IndirectTableKeyIterator&) = default;
    auto operator=(const IndirectTableKeyIterator&) -> IndirectTableKeyIterator& = default;
    IndirectTableKeyIterator(IndirectTableKeyIterator&&) noexcept = default;
    auto operator=(IndirectTableKeyIterator&&) noexcept -> IndirectTableKeyIterator& = default;

    ~IndirectTableKeyIterator() = default;

    // Prefix
    auto operator++() -> IndirectTableKeyIterator&;
    // Postfix
    auto operator++(int) -> IndirectTableKeyIterator;

    // Prefix
    auto operator--() -> IndirectTableKeyIterator&;
    // Postfix
    auto operator--(int) -> IndirectTableKeyIterator;

    auto operator*() -> reference;
    auto operator->() -> pointer;

    bool operator==(const IndirectTableKeyIterator& a) const;
    bool operator!=(const IndirectTableKeyIterator& a) const;

    void swap(IndirectTableKeyIterator& other);

    using ref_to_table_value_type = std::conditional_t<std::is_const_v<TableType>,
                                                       typename TableType::const_reference,
                                                       typename TableType::reference>;

    /**
     * @brief Query the value at the iterator's current key
     *
     * @return Component
     */
    auto queryValue() -> ref_to_table_value_type&;

private:
    inline void incrementCurrentKey();
    inline void decrementCurrentKey();

    typename std::vector<size_t>::const_iterator it;
    TableType* table;

    key_type currentKey;
};



// ------------------------ //
//      Value Iterator      //
// ------------------------ //

template<typename TableType>
inline IndirectTableValueIterator<TableType>::IndirectTableValueIterator(VectorIterator _it)
    :
    it(_it)
{
}

template<typename TableType>
inline auto IndirectTableValueIterator<TableType>::operator*() -> reference
{
    return *it;
}

template<typename TableType>
inline auto IndirectTableValueIterator<TableType>::operator->() -> pointer
{
    return &*it;
}

template<typename TableType>
inline auto IndirectTableValueIterator<TableType>::operator++() -> IndirectTableValueIterator&
{
    ++it;
    return *this;
}

template<typename TableType>
inline auto IndirectTableValueIterator<TableType>::operator++(int) -> IndirectTableValueIterator
{
    auto result = *this;
    ++*this;
    return result;
}

template<typename TableType>
inline auto IndirectTableValueIterator<TableType>::operator--() -> IndirectTableValueIterator&
{
    --it;
    return *this;
}

template<typename TableType>
inline auto IndirectTableValueIterator<TableType>::operator--(int) -> IndirectTableValueIterator
{
    auto result = *this;
    --*this;
    return result;
}

template<typename TableType>
inline bool IndirectTableValueIterator<TableType>::operator==(const IndirectTableValueIterator& other) const
{
    return other.it == it;
}

template<typename TableType>
inline bool IndirectTableValueIterator<TableType>::operator!=(const IndirectTableValueIterator& other) const
{
    return other.it != it;
}

template<typename TableType>
inline void IndirectTableValueIterator<TableType>::swap(IndirectTableValueIterator& other)
{
    std::swap(other.it, it);
}



// ---------------------- //
//      Key Iterator      //
// ---------------------- //

template<typename TableType>
inline IndirectTableKeyIterator<TableType>::IndirectTableKeyIterator(
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
inline auto IndirectTableKeyIterator<TableType>::operator*() -> reference
{
    return currentKey;
}

template<typename TableType>
inline auto IndirectTableKeyIterator<TableType>::operator->() -> pointer
{
    return &currentKey;
}

template<typename TableType>
inline auto IndirectTableKeyIterator<TableType>::operator++() -> IndirectTableKeyIterator&
{
    do {
        ++it;
        incrementCurrentKey();
    } while (it < table->indices.end() && *it == TableType::NONE);
    return *this;
}

template<typename TableType>
inline auto IndirectTableKeyIterator<TableType>::operator++(int) -> IndirectTableKeyIterator
{
    auto result = *this;
    ++*this;
    return result;
}

template<typename TableType>
inline auto IndirectTableKeyIterator<TableType>::operator--() -> IndirectTableKeyIterator&
{
    while (it >= table->indices.begin() && *it == TableType::NONE)
    {
        --it;
        decrementCurrentKey();
    }
    return *this;
}

template<typename TableType>
inline auto IndirectTableKeyIterator<TableType>::operator--(int) -> IndirectTableKeyIterator
{
    auto result = *this;
    --*this;
    return result;
}

template<typename TableType>
inline bool IndirectTableKeyIterator<TableType>::operator==(const IndirectTableKeyIterator& other) const
{
    return other.it == it;
}

template<typename TableType>
inline bool IndirectTableKeyIterator<TableType>::operator!=(const IndirectTableKeyIterator& other) const
{
    return other.it != it;
}

template<typename TableType>
inline void IndirectTableKeyIterator<TableType>::swap(IndirectTableKeyIterator& other)
{
    std::swap(other.it, it);
    std::swap(other.table, table);
    std::swap(other.currentKey, currentKey);
}

template<typename TableType>
inline auto IndirectTableKeyIterator<TableType>::queryValue() -> ref_to_table_value_type&
{
    assert(table->at(currentKey) != nullptr);
    return *table->at(currentKey);
}

template<typename TableType>
inline void IndirectTableKeyIterator<TableType>::incrementCurrentKey()
{
    currentKey = key_type(static_cast<size_t>(currentKey) + 1);
}

template<typename TableType>
inline void IndirectTableKeyIterator<TableType>::decrementCurrentKey()
{
    currentKey = key_type(static_cast<size_t>(currentKey) - 1);
}

} // namespace componentlib
