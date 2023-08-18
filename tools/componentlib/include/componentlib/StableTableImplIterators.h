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
        using iterator_category = std::forward_iterator_tag;

        using value_type = typename TableType::value_type;
        using reference = typename TableType::reference;
        using const_reference = typename TableType::const_reference;
        using pointer = typename TableType::pointer;
        using const_pointer = typename TableType::const_pointer;
        using key_type = typename TableType::key_type;

        using size_type = typename TableType::size_type;

        using conditionally_const_value_type = std::conditional_t<
            std::is_const_v<TableType>,
            const typename TableType::value_type,
            typename TableType::value_type
        >;

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
    struct StableTableValueIterator
        : public StableTableIterator<TableType>
    {
        using Base = StableTableIterator<TableType>;
        using key_type = typename Base::key_type;

        using typename Base::conditionally_const_value_type;

        StableTableValueIterator(TableType& table, key_type key) : Base(table, key) {}

        auto operator*() -> conditionally_const_value_type&
        {
            assert(Base::table->contains(Base::key));
            assert(Base::table->at(Base::key) != nullptr);
            return *Base::table->at(Base::key);
        }

        auto operator->() -> conditionally_const_value_type*
        {
            assert(Base::table->contains(Base::key));
            return Base::table->at(Base::key);

        }
    };

    template<typename TableType>
    struct StableTableKeyIterator
        : public StableTableIterator<TableType>
    {
        using Base = StableTableIterator<TableType>;
        using key_type = typename Base::key_type;

        using typename Base::conditionally_const_value_type;

        StableTableKeyIterator(TableType& table, key_type key) : Base(table, key) {}

        auto operator*() -> const key_type&
        {
            assert(Base::table->contains(Base::key));
            assert(Base::table->at(Base::key) != nullptr);
            return Base::key;
        }

        auto operator->() -> const key_type*
        {
            assert(Base::table->contains(Base::key));
            return &Base::key;

        }

        auto queryValue() -> conditionally_const_value_type&
        {
            return *Base::table->at(Base::key);
        }
    };
} // namespace componentlib
