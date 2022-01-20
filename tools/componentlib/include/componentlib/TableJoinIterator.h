#pragma once

#include <iostream>

#include "TableIterators.h"
#include "IteratorRange.h"

namespace componentlib
{

/**
 * @brief An iterator that dynamically joins two tables
 */
template<typename TableT, typename TableU>
    requires std::convertible_to<typename TableT::key_type, typename TableU::key_type>
          && std::convertible_to<typename TableU::key_type, typename TableT::key_type>
struct TableJoinIterator
{
    using T = typename TableT::value_type;
    using U = typename TableU::value_type;
    using KeyT = typename TableT::key_type;
    using KeyU = typename TableU::key_type;

    using KeyIteratorT = TableKeyIterator<TableT>;
    using KeyIteratorU = TableKeyIterator<TableU>;

    using value_type = std::pair<T*, U*>;
    using reference = value_type&;
    using pointer = value_type*;

    /**
     * I don't have bidirectionality because I couldn't figure out how to
     * decrement the past-the-end iterator.
     */
    using iterator_category = std::forward_iterator_tag;

    /**
     * A structure representing a single row in a join of two tables
     */
    struct RowJoin
    {
        KeyT key;
        T& t;
        U& u;
    };

    /**
     * A structure representing a single row in a join of two tables
     */
    struct ConstRowJoin
    {
        const KeyT key;
        const T& t;
        const U& u;
    };

    /**
     * @brief Construct a join iterator over to tables from their beginning
     */
    TableJoinIterator(TableT& t, TableU& u)
        : TableJoinIterator(t.keys(), u.keys())
    {}

    /**
     * @brief Construct a join iterator from two key iterators
     */
    TableJoinIterator(IteratorRange<KeyIteratorT> t, IteratorRange<KeyIteratorU> u)
        : it_t(std::begin(t)), it_u(std::begin(u))
    {
        while (it_t != std::end(t) && *it_t < *it_u) ++it_t;
        while (it_u != std::end(u) && *it_u < *it_t) ++it_u;

        // If one of the iterators is end, set both to end
        if (it_t == std::end(t) || it_u == std::end(u))
        {
            it_t = std::end(t);
            it_u = std::end(u);
        }
    }

    ~TableJoinIterator() = default;

    // Prefix
    auto operator++() -> TableJoinIterator&
    {
        findNextMatch();
        return *this;
    }

    // Postfix
    auto operator++(int) -> TableJoinIterator
    {
        auto result = *this;
        ++this;
        return result;
    }

    auto operator*() -> RowJoin
    {
        return { *it_t, it_t.queryValue(), it_u.queryValue() };
    }

    auto operator*() const -> ConstRowJoin
    {
        return { *it_t, it_t.table->get(*it_t), it_u.table->get(*it_u) };
    }

    bool operator==(const TableJoinIterator& other) const
    {
        return it_t == other.it_t || it_u == other.it_u;
    }

    bool operator!=(const TableJoinIterator& other) const
    {
        return !(*this == other);
    }

    void swap(TableJoinIterator& other)
    {
        std::swap(it_t, other.it_t);
        std::swap(it_u, other.it_u);
    }

private:
    void findNextMatch()
    {
        do {
            ++it_u;
            while (*it_t < *it_u) {
                ++it_t;
            }
        } while (*it_t != *it_u);
    }

    KeyIteratorT it_t;
    KeyIteratorU it_u;
};

} // namespace componentlib
