#pragma once

#include <cassert>
#include <concepts>
#include <iterator>

namespace componentlib
{
    template<typename TableType>
    class StableTableIterator
    {
    public:
        using key_type = typename TableType::key_type;
        using size_type = typename TableType::size_type;

        StableTableIterator(TableType& _table, key_type keyPos)
            :
            table(&_table),
            key(keyPos)
        {
            while (table->chunkIndex(key) < table->chunks.size() && !table->contains(key)) {
                key = inc(key);
            }

            assert(table->contains(key)
                   || static_cast<size_type>(key) == (table->chunks.size() * table->kChunkSize));
        }

        auto operator++() -> StableTableIterator&
        {
            do {
                key = inc(key);
            } while (table->chunkIndex(key) < table->chunks.size()
                     && !table->chunks.at(table->chunkIndex(key))->valid(table->elemIndex(key)));

            return *this;
        }

        auto operator++(int) -> StableTableIterator
        {
            auto ret = *this;
            ++*this;
            return ret;
        }

        bool operator==(const StableTableIterator&) const = default;
        bool operator!=(const StableTableIterator&) const = default;

    protected:
        static constexpr auto inc(key_type key) -> key_type {
            return key_type(static_cast<size_type>(key) + 1);
        }

        TableType* table;
        key_type key;
    };

    template<typename TableType>
    struct StableTableValueIterator : public StableTableIterator<TableType>
    {
        using iterator_category = std::forward_iterator_tag;

        using Base = StableTableIterator<TableType>;

        using value_type = typename TableType::value_type;
        using conditionally_const_value_type = std::conditional_t<
            std::is_const_v<TableType>,
            std::add_const_t<value_type>,
            value_type
        >;
        using reference = conditionally_const_value_type&;
        using pointer = conditionally_const_value_type*;
        using typename Base::key_type;

        StableTableValueIterator(TableType& table, key_type key) : Base(table, key) {}

        auto operator*() -> reference
        {
            assert(Base::table->contains(Base::key));
            assert(Base::table->at(Base::key) != nullptr);
            return *Base::table->at(Base::key);
        }

        auto operator->() -> pointer
        {
            assert(Base::table->contains(Base::key));
            return Base::table->at(Base::key);

        }
    };

    template<typename TableType>
    struct StableTableKeyIterator : public StableTableIterator<TableType>
    {
        using iterator_category = std::forward_iterator_tag;

        using Base = StableTableIterator<TableType>;

        using typename Base::key_type;
        using value_type = key_type;
        using reference = const key_type&;
        using pointer = const key_type*;

        StableTableKeyIterator(TableType& table, key_type key) : Base(table, key) {}

        auto operator*() -> reference
        {
            assert(Base::table->contains(Base::key));
            assert(Base::table->at(Base::key) != nullptr);
            return Base::key;
        }

        auto operator->() -> pointer
        {
            assert(Base::table->contains(Base::key));
            return &Base::key;

        }

        auto queryValue() -> std::conditional_t<std::is_const_v<TableType>,
                                                typename TableType::const_reference,
                                                typename TableType::reference>
        {
            return *Base::table->at(Base::key);
        }
    };
} // namespace componentlib
