#pragma once

#include <cassert>
#include <iterator>
#include <type_traits>

namespace componentlib
{
    template<typename TableType, typename Derived>
    class StableTableIterator
    {
    public:
        struct Sentinel {};

        using iterator_category = std::input_iterator_tag;

        using key_type = typename TableType::key_type;
        using size_type = typename TableType::size_type;
        using difference_type = std::ptrdiff_t;

        StableTableIterator() = default;

        StableTableIterator(TableType& _table, key_type keyPos)
            :
            table(&_table),
            key(keyPos)
        {
            while (*this != Sentinel{} && !table->contains(key)) {
                key = inc(key);
            }

            assert(table->contains(key) || *this == Sentinel{});
        }

        auto operator++() -> Derived&
        {
            do {
                key = inc(key);
            } while (*this != Sentinel{} && !table->contains(key));

            return static_cast<Derived&>(*this);
        }

        auto operator++(int) -> Derived
        {
            auto ret = static_cast<Derived&>(*this);
            ++*this;
            return ret;
        }

        bool operator==(const StableTableIterator&) const = default;
        bool operator!=(const StableTableIterator&) const = default;

        bool operator==(const Sentinel&) const noexcept {
            return table == nullptr || static_cast<size_t>(key) == table->capacity();
        }

        bool operator!=(const Sentinel&) const noexcept {
            return !(*this == Sentinel{});
        }

    protected:
        static constexpr auto inc(key_type key) -> key_type {
            return key_type(static_cast<size_type>(key) + 1);
        }

        TableType* table{ nullptr };
        key_type key{ 0 };
    };

    template<typename TableType>
    struct StableTableValueIterator
        : public StableTableIterator<TableType, StableTableValueIterator<TableType>>
    {
        using iterator_category = std::forward_iterator_tag;

        using Base = StableTableIterator<TableType, StableTableValueIterator<TableType>>;

        using value_type = typename TableType::value_type;
        using conditionally_const_value_type = std::conditional_t<
            std::is_const_v<TableType>,
            std::add_const_t<value_type>,
            value_type
        >;
        using reference = conditionally_const_value_type&;
        using pointer = conditionally_const_value_type*;
        using typename Base::key_type;

        StableTableValueIterator() = default;
        StableTableValueIterator(TableType& table, key_type key) : Base(table, key) {}

        auto operator*(this auto&& self) -> reference
        {
            assert(self.table->contains(self.key));
            assert(self.table->at(self.key) != nullptr);
            return *self.table->at(self.key);
        }

        auto operator->(this auto&& self) -> pointer
        {
            assert(self.table->contains(self.key));
            return self.table->at(self.key);

        }

        bool operator==(const StableTableValueIterator&) const = default;
        bool operator!=(const StableTableValueIterator&) const = default;
    };

    template<typename TableType>
    struct StableTableKeyIterator
        : public StableTableIterator<TableType, StableTableKeyIterator<TableType>>
    {
        using iterator_category = std::forward_iterator_tag;

        using Base = StableTableIterator<TableType, StableTableKeyIterator<TableType>>;

        using typename Base::key_type;
        using value_type = key_type;
        using reference = const key_type&;
        using pointer = const key_type*;

        StableTableKeyIterator() = default;
        StableTableKeyIterator(TableType& table, key_type key) : Base(table, key) {}

        auto operator*(this auto&& self) -> reference
        {
            assert(self.table->contains(self.key));
            assert(self.table->at(self.key) != nullptr);
            return self.key;
        }

        auto operator->(this auto&& self) -> pointer
        {
            assert(self.table->contains(self.key));
            return &self.key;

        }

        bool operator==(const StableTableKeyIterator&) const = default;
        bool operator!=(const StableTableKeyIterator&) const = default;

        auto queryValue() -> std::conditional_t<std::is_const_v<TableType>,
                                                typename TableType::const_reference,
                                                typename TableType::reference>
        {
            return *Base::table->at(Base::key);
        }
    };
} // namespace componentlib
