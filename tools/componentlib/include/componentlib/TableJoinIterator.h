#pragma once

#include <iostream>

#include <trc_util/algorithm/IteratorRange.h>

#include "TableIterators.h"

namespace componentlib
{

using trc::algorithm::IteratorRange;

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

    using KeyIteratorT = typename TableT::KeyIterator;
    using KeyIteratorU = typename TableU::KeyIterator;

    /**
     * A structure representing a single row in a join of two tables
     */
    struct RowJoin
    {
        KeyT key;
        T& t;
        U& u;
    };

    using difference_type = size_t;
    using value_type = RowJoin;
    using reference = RowJoin;

    /**
     * I don't have bidirectionality because I couldn't figure out how to
     * decrement the past-the-end iterator.
     */
    using iterator_category = std::forward_iterator_tag;

    TableJoinIterator(const TableJoinIterator&) = default;
    TableJoinIterator(TableJoinIterator&&) noexcept = default;
    ~TableJoinIterator() = default;

    TableJoinIterator& operator=(const TableJoinIterator&) = default;
    TableJoinIterator& operator=(TableJoinIterator&&) noexcept = default;

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
        :
        it_t(std::begin(t)), it_u(std::begin(u)),
        end_t(std::end(t)), end_u(std::end(u))
    {
        // Try to catch `u` up to `t`
        while (it_t != std::end(t) && it_u != std::end(u) && *it_u < *it_t) ++it_u;

        // If one of the iterators is end, set both to end
        if (it_t == std::end(t) || it_u == std::end(u))
        {
            it_t = std::end(t);
            it_u = std::end(u);
            return;
        }

        // `u` is larger than `t`. Try to find the next join pair.
        if (*it_t != *it_u) {
            findNextMatch();
        }
    }

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

    bool operator==(const TableJoinIterator& other) const
    {
        return it_t == other.it_t && it_u == other.it_u;
    }

    bool operator!=(const TableJoinIterator& other) const
    {
        return !(*this == other);
    }

    void swap(TableJoinIterator& other)
    {
        std::swap(it_t, other.it_t);
        std::swap(it_u, other.it_u);
        std::swap(end_t, other.end_t);
        std::swap(end_u, other.end_u);
    }

private:
    void findNextMatch()
    {
        // Ensure correctness of the implementation
        assert((it_t == end_t && it_u == end_u)
            || (it_t != end_t && it_u != end_u));

        // A courtesy to detect disallowed usage
        assert(it_t != end_t && "Tried to increment a past-the-end iterator.");

        do {
            // First, increment `t`.
            ++it_t;
            if (it_t == end_t)  // If either is `end`, set both to `end`.
            {
                it_u = end_u;
                break;
            }

            // Now try to catch `u` up to `t`.
            while (it_u != end_u && *it_u < *it_t) ++it_u;
            if (it_u == end_u)  // If either is `end`, set both to `end`.
            {
                it_t = end_t;
                break;
            }
        } while (*it_u != *it_t);
    }

    KeyIteratorT it_t;
    KeyIteratorU it_u;

    KeyIteratorT end_t;
    KeyIteratorU end_u;
};

} // namespace componentlib
