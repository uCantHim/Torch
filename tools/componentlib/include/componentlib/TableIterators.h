#pragma once

#include <iterator>
#include <vector>

#include "TableBase.h"

namespace componentlib
{

template<typename TableType>
struct TablePairIterator;

template<typename T>
constexpr bool isConst = !std::is_same_v<T, std::remove_cv_t<T>>;

/**
 * @brief Iterator over keys in a table
 */
template<typename TableType>
struct TableValueIterator
{
private:
    using VectorIterator = std::conditional_t<
        std::is_const_v<TableType>,
        typename std::vector<typename TableType::StoredType>::const_iterator,
        typename std::vector<typename TableType::StoredType>::iterator
    >;

public:
    using value_type = typename TableType::value_type;
    using key_type = typename TableType::key_type;
    using reference = value_type&;
    using pointer = value_type*;

    using difference_type = size_t;

    // Bidirectional for now. LegacyRandomAccessIterator is much more complex
    using iterator_category = std::bidirectional_iterator_tag;

    TableValueIterator() = default;
    explicit
    TableValueIterator(VectorIterator it);

    TableValueIterator(const TableValueIterator&) = default;
    auto operator=(const TableValueIterator&) -> TableValueIterator& = default;
    TableValueIterator(TableValueIterator&&) noexcept = default;
    auto operator=(TableValueIterator&&) noexcept -> TableValueIterator& = default;

    ~TableValueIterator() = default;

    // Prefix
    auto operator++() -> TableValueIterator&;
    // Postfix
    auto operator++(int) -> TableValueIterator;

    // Prefix
    auto operator--() -> TableValueIterator&;
    // Postfix
    auto operator--(int) -> TableValueIterator;

    auto operator*()       -> value_type&
        requires (!std::is_const_v<TableType>);
    auto operator*() const -> const value_type&;
    auto operator->()       -> value_type*
        requires (!std::is_const_v<TableType>);
    auto operator->() const -> const value_type*;

    bool operator==(const TableValueIterator& a) const;
    bool operator!=(const TableValueIterator& a) const;

    void swap(TableValueIterator& other);

private:
    auto getCurrentRef() -> value_type&;
    auto getCurrentRef() const -> const value_type&;
    VectorIterator it;
};
/**
 * @brief Iterator over keys in a table
 */
template<typename TableType>
struct TableKeyIterator
{
    using value_type = typename TableType::value_type;
    using conditionally_const_value_type
        = std::conditional_t<isConst<TableType>, const value_type, value_type>;
    using key_type = typename TableType::key_type;
    using reference = value_type&;
    using pointer = value_type*;

    using difference_type = size_t;

    // Bidirectional for now. LegacyRandomAccessIterator is much more complex
    using iterator_category = std::bidirectional_iterator_tag;

    TableKeyIterator() = default;
    TableKeyIterator(typename std::vector<size_t>::const_iterator it, TableType& table);

    TableKeyIterator(const TableKeyIterator&) = default;
    auto operator=(const TableKeyIterator&) -> TableKeyIterator& = default;
    TableKeyIterator(TableKeyIterator&&) noexcept = default;
    auto operator=(TableKeyIterator&&) noexcept -> TableKeyIterator& = default;

    ~TableKeyIterator() = default;

    // Prefix
    auto operator++() -> TableKeyIterator&;
    // Postfix
    auto operator++(int) -> TableKeyIterator;

    // Prefix
    auto operator--() -> TableKeyIterator&;
    // Postfix
    auto operator--(int) -> TableKeyIterator;

    auto operator*()       -> key_type&;
    auto operator*() const -> const key_type&;
    auto operator->()       -> key_type*;
    auto operator->() const -> const key_type*;

    bool operator==(const TableKeyIterator& a) const;
    bool operator!=(const TableKeyIterator& a) const;

    void swap(TableKeyIterator& other);

    /**
     * @brief Query the value at the iterator's current key
     *
     * @return Component
     */
    auto queryValue() -> conditionally_const_value_type&;

private:
    inline void incrementCurrentKey();
    inline void decrementCurrentKey();

    typename std::vector<size_t>::const_iterator it;
    TableType* table;

    key_type currentKey;
};

/**
 * @brief Iterator over key/value pairs in a table
 */
template<typename TableType>
struct TablePairIterator
{
    using value_type = typename TableType::value_type;
    using conditionally_const_value_type
        = typename TableKeyIterator<TableType>::conditionally_const_value_type;
    using key_type = typename TableType::key_type;
    using reference = value_type&;
    using pointer = value_type*;

    using difference_type = size_t;

    // Bidirectional for now. LegacyRandomAccessIterator is much more complex
    using iterator_category = std::bidirectional_iterator_tag;

    struct KeyValuePair
    {
        key_type key;
        conditionally_const_value_type& value;
    };

    TablePairIterator() = default;
    explicit TablePairIterator(TableKeyIterator<TableType> it);

    TablePairIterator(const TablePairIterator&) = default;
    auto operator=(const TablePairIterator&) -> TablePairIterator& = default;
    TablePairIterator(TablePairIterator&&) noexcept = default;
    auto operator=(TablePairIterator&&) noexcept -> TablePairIterator& = default;

    ~TablePairIterator() = default;

    // Prefix
    auto operator++() -> TablePairIterator&;
    // Postfix
    auto operator++(int) -> TablePairIterator;

    // Prefix
    auto operator--() -> TablePairIterator&;
    // Postfix
    auto operator--(int) -> TablePairIterator;

    auto operator*()       -> KeyValuePair;
    auto operator*() const -> const KeyValuePair;

    bool operator==(const TablePairIterator& a) const;
    bool operator!=(const TablePairIterator& a) const;

    void swap(TablePairIterator& other);

private:
    TableKeyIterator<TableType> keyIt;
};


#include "TableIterators.inl"

} // namespace componentlib
