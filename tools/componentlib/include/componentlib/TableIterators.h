#pragma once

#include <concepts>
#include <iterator>
#include <type_traits>

namespace componentlib
{

template<typename TableType>
struct TablePairIterator;

/**
 * @brief Iterator over key/value pairs in a table
 */
template<typename TableType>
struct TablePairIterator
{
private:
    using KeyIterator = std::conditional_t<
        std::is_const_v<TableType>,
        typename TableType::ConstKeyIterator,
        typename TableType::KeyIterator
    >;

public:
    using value_type = typename TableType::value_type;
    using key_type = typename TableType::key_type;
    using reference = value_type&;
    using pointer = value_type*;

    using difference_type = size_t;

    using conditionally_const_value_type = std::conditional_t<
        std::is_const_v<TableType>,
        const value_type,
        value_type
    >;

    // Bidirectional for now. LegacyRandomAccessIterator is much more complex
    using iterator_category = std::bidirectional_iterator_tag;

    struct KeyValuePair
    {
        key_type key;
        conditionally_const_value_type& value;
    };

    TablePairIterator() = default;
    explicit TablePairIterator(KeyIterator it);

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
    KeyIterator keyIt;
};



// ----------------------- //
//      Pair Iterator      //
// ----------------------- //

template<typename TableType>
inline TablePairIterator<TableType>::TablePairIterator(KeyIterator it)
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

} // namespace componentlib
