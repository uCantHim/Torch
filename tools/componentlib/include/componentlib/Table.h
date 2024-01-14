#pragma once

#include <cassert>
#include <concepts>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>

#include <trc_util/algorithm/IteratorRange.h>
#include <trc_util/functional/Maybe.h>
namespace trc {
    using functional::Maybe;
}

#include "IndirectTableImpl.h"
#include "StableTableImpl.h"
#include "TableIterators.h"
#include "TableJoinIterator.h"

namespace componentlib
{

using trc::algorithm::IteratorRange;

/**
 * Database-style table interface. May be backed by different implementations
 * based on the tradeoffs you want to make.
 */
template<typename T, typename Key = uint32_t, typename Impl = StableTableImpl<T, Key>>
class Table
{
public:
    using value_type = T;
    using key_type = Key;

    using reference = T&;
    using const_reference = const T&;
    using pointer = T*;
    using const_pointer = const T*;

    using size_type = size_t;

public:
    Table() requires std::is_default_constructible_v<Impl> = default;

    template<typename ...Args> requires std::constructible_from<Impl, Args...>
    explicit Table(Args&&... args)
        : impl(std::forward<Args>(args)...)
    {}

    Table(const Table&) requires std::copy_constructible<Impl> = default;
    Table(Table&&) noexcept requires std::move_constructible<Impl> = default;
    Table& operator=(const Table&) requires std::is_copy_assignable_v<Impl> = default;
    Table& operator=(Table&&) noexcept requires std::is_move_assignable_v<Impl> = default;
    ~Table() noexcept requires std::destructible<Impl> = default;

public:
    // ------------------------ //
    //      Element access      //
    // ------------------------ //

    /**
     * @brief Determine whether an object with a sepcific key exists in the
     *        table
     *
     * @param Key key
     *
     * @return bool True if an object with key `key` exists in the table,
     *              false otherwise.
     */
    inline bool contains(key_type key) const;

    /**
     * @param Key key
     *
     * @throw std::out_of_range if no object with key `key` exists in the
     *                          table.
     */
    inline auto get(key_type key) -> reference;

    /**
     * @param Key key
     *
     * @throw std::out_of_range if no object with key `key` exists in the
     *                          table.
     */
    inline auto get(key_type key) const -> const_reference;

    inline auto try_get(key_type key) -> pointer;
    inline auto try_get(key_type key) const -> const_pointer;

    inline auto get_m(key_type key) -> trc::Maybe<reference>;
    inline auto get_m(key_type key) const -> trc::Maybe<const_reference>;

public:
    // --------------------------- //
    //      Element insertion      //
    // --------------------------- //

    /**
     * @brief Insert an object at a specific key
     *
     * Constructs the object in-place. If an object exists at `key`, it is
     * overwritten.
     */
    template<typename ...Args>
        requires std::constructible_from<value_type, Args&&...>
    inline auto emplace(key_type key, Args&&... args) -> reference;

    /**
     * @brief Try to insert an object at a specific key
     *
     * Constructs the object in-place. Does not overwrite existing objects
     * at `key`.
     *
     * @return std::pair<reference, bool> The new object and `true` if the
     *         insertion was successful. The existing object and `false`
     *         otherwise.
     */
    template<typename ...Args>
        requires std::constructible_from<value_type, Args&&...>
    inline auto try_emplace(key_type key, Args&&... args) -> std::pair<reference, bool>;

public:
    // ------------------------- //
    //      Element erasure      //
    // ------------------------- //

    /**
     * @brief Throws if no object exists at key `key`
     *
     * @param Key key
     *
     * @return T The erased object at key `key`.
     * @throw std::out_of_range if no object exists at key `key`
     */
    inline auto erase(key_type key) -> value_type;

    /**
     * @brief Try to erase an object
     *
     * @param Key key
     *
     * Does nothing if no object with the specified key exists.
     *
     * @return std::optional<T> The erased T if an object
     *                                  was erased, nullopt otherwise.
     */
    inline auto try_erase(key_type key) noexcept -> std::optional<value_type>;

    /**
     * @brief Try to erase an object
     *
     * @param Key key
     *
     * @return trc::Maybe<T> Nothing if no object was erased, the
     *                               erased object otherwise.
     */
    inline auto erase_m(key_type key) noexcept -> trc::Maybe<value_type>;

    /**
     * @brief Erase all objects and keys from the table
     */
    inline void clear() noexcept;

public:
    // ------------------------- //
    //      Storage control      //
    // ------------------------- //

    /**
     * @brief Reserve a minimum amount of storage space for elements
     *
     * @param size_type minSize Pre-allocate enough storage for `minSize`
     *                          elements.
     */
    inline void reserve(size_type minSize);

public:
    // -------------------------- //
    //      Iterator classes      //
    // -------------------------- //

    // iterator types
    using ValueIterator = typename Impl::ValueIterator;
    using KeyIterator = typename Impl::KeyIterator;
    using PairIterator = TablePairIterator<Table<T, Key, Impl>>;
    template<typename Other>
    using JoinIterator = TableJoinIterator<Table<T, Key, Impl>, Other>;

    // const-iterator types
    using ConstValueIterator = typename Impl::ConstValueIterator;
    using ConstKeyIterator = typename Impl::ConstKeyIterator;
    using ConstPairIterator = TablePairIterator<const Table<T, Key, Impl>>;
    template<typename Other>
    using ConstJoinIterator = TableJoinIterator<const Table<T, Key, Impl>, Other>;

    // STL-compliant default typedefs
    using iterator = ValueIterator;
    using const_iterator = ConstValueIterator;

public:
    // ----------------------------------- //
    //      Iterator access functions      //
    // ----------------------------------- //

    auto begin()       -> ValueIterator;
    auto begin() const -> ConstValueIterator;
    auto end()         -> ValueIterator;
    auto end()   const -> ConstValueIterator;

    auto keyBegin()       -> KeyIterator;
    auto keyBegin() const -> ConstKeyIterator;
    auto keyEnd()         -> KeyIterator;
    auto keyEnd()   const -> ConstKeyIterator;

    auto values()       -> IteratorRange<ValueIterator>;
    auto values() const -> IteratorRange<ConstValueIterator>;
    auto keys()         -> IteratorRange<KeyIterator>;
    auto keys()   const -> IteratorRange<ConstKeyIterator>;
    auto items()        -> IteratorRange<PairIterator>;
    auto items()  const -> IteratorRange<ConstPairIterator>;

    template<typename TableType>
    auto join(TableType& other) -> IteratorRange<JoinIterator<TableType>>;

    template<typename TableType>
    auto join(const TableType& other) const -> IteratorRange<ConstJoinIterator<TableType>>;

private:
    Impl impl;
};



// -------------------------------- //
//      Table implementations       //
// -------------------------------- //

template<typename T, typename Key, typename Impl>
inline bool Table<T, Key, Impl>::contains(key_type key) const
{
    return impl.contains(key);
}

template<typename T, typename Key, typename Impl>
inline auto Table<T, Key, Impl>::get(key_type key) -> reference
{
    if (pointer val = impl.at(key)) {
        return *val;
    }
    throw std::out_of_range("");
}

template<typename T, typename Key, typename Impl>
inline auto Table<T, Key, Impl>::get(key_type key) const -> const_reference
{
    if (const_pointer val = impl.at(key)) {
        return *val;
    }
    throw std::out_of_range("");
}

template<typename T, typename Key, typename Impl>
inline auto Table<T, Key, Impl>::try_get(key_type key) -> pointer
{
    return impl.at(key);
}

template<typename T, typename Key, typename Impl>
inline auto Table<T, Key, Impl>::try_get(key_type key) const -> const_pointer
{
    return impl.at(key);
}

template<typename T, typename Key, typename Impl>
inline auto Table<T, Key, Impl>::get_m(Key key) -> trc::Maybe<reference>
{
    if (!contains(key)) {
        return {};
    }
    return get(key);
}

template<typename T, typename Key, typename Impl>
inline auto Table<T, Key, Impl>::get_m(Key key) const -> trc::Maybe<const_reference>
{
    if (!contains(key)) {
        return {};
    }
    return get(key);
}

template<typename T, typename Key, typename Impl>
template<typename ...Args>
    requires std::constructible_from<T, Args&&...>
inline auto Table<T, Key, Impl>::emplace(key_type key, Args&&... args) -> reference
{
    return impl.emplace(key, std::forward<Args>(args)...);
}

template<typename T, typename Key, typename Impl>
template<typename ...Args>
    requires std::constructible_from<T, Args&&...>
inline auto Table<T, Key, Impl>::try_emplace(key_type key, Args&&... args) -> std::pair<reference, bool>
{
    if (impl.contains(key)) {
        return { *impl.at(key), false };
    }

    return { impl.emplace(key, std::forward<Args>(args)...), true };
}

template<typename T, typename Key, typename Impl>
inline auto Table<T, Key, Impl>::erase(Key key) -> value_type
{
    if (!impl.contains(key)) {
        throw std::out_of_range("In Table<>::erase: No object exists at key "
                                + std::to_string(static_cast<size_type>(key)) + "");
    }

    value_type ret = std::move(*impl.at(key));
    impl.erase(key);
    return ret;
}

template<typename T, typename Key, typename Impl>
inline auto Table<T, Key, Impl>::try_erase(Key key) noexcept -> std::optional<value_type>
{
    if (!impl.contains(key)) {
        return std::nullopt;
    }

    value_type ret = std::move(*impl.at(key));
    impl.erase(key);
    return ret;
}

template<typename T, typename Key, typename Impl>
inline auto Table<T, Key, Impl>::erase_m(Key key) noexcept -> trc::Maybe<value_type>
{
    if (!impl.contains(key)) {
        return {};
    }

    value_type ret = std::move(*impl.at(key));
    impl.erase(key);
    return ret;
}

template<typename T, typename Key, typename Impl>
inline void Table<T, Key, Impl>::clear() noexcept
{
    impl.clear();
}

template<typename T, typename Key, typename Impl>
inline void Table<T, Key, Impl>::reserve(size_t minSize)
{
    impl.reserve(minSize);
}

template<typename T, typename Key, typename Impl>
inline auto Table<T, Key, Impl>::begin() -> ValueIterator
{
    return impl.valueBegin();
}

template<typename T, typename Key, typename Impl>
inline auto Table<T, Key, Impl>::begin() const -> ConstValueIterator
{
    return impl.valueBegin();
}

template<typename T, typename Key, typename Impl>
inline auto Table<T, Key, Impl>::end() -> ValueIterator
{
    return impl.valueEnd();
}

template<typename T, typename Key, typename Impl>
inline auto Table<T, Key, Impl>::end() const -> ConstValueIterator
{
    return impl.valueEnd();
}

template<typename T, typename Key, typename Impl>
inline auto Table<T, Key, Impl>::keyBegin() -> KeyIterator
{
    return impl.keyBegin();
}

template<typename T, typename Key, typename Impl>
inline auto Table<T, Key, Impl>::keyBegin() const -> ConstKeyIterator
{
    return impl.keyBegin();
}

template<typename T, typename Key, typename Impl>
inline auto Table<T, Key, Impl>::keyEnd() -> KeyIterator
{
    return impl.keyEnd();
}

template<typename T, typename Key, typename Impl>
inline auto Table<T, Key, Impl>::keyEnd() const -> ConstKeyIterator
{
    return impl.keyEnd();
}

template<typename T, typename Key, typename Impl>
inline auto Table<T, Key, Impl>::values() -> IteratorRange<ValueIterator>
{
    return { begin(), end() };
}

template<typename T, typename Key, typename Impl>
inline auto Table<T, Key, Impl>::values() const -> IteratorRange<ConstValueIterator>
{
    return { begin(), end() };
}

template<typename T, typename Key, typename Impl>
inline auto Table<T, Key, Impl>::keys() -> IteratorRange<KeyIterator>
{
    return { keyBegin(), keyEnd() };
}

template<typename T, typename Key, typename Impl>
inline auto Table<T, Key, Impl>::keys() const -> IteratorRange<ConstKeyIterator>
{
    return { keyBegin(), keyEnd() };
}

template<typename T, typename Key, typename Impl>
inline auto Table<T, Key, Impl>::items() -> IteratorRange<PairIterator>
{
    return { PairIterator(keyBegin()), PairIterator(keyEnd()) };
}

template<typename T, typename Key, typename Impl>
inline auto Table<T, Key, Impl>::items() const -> IteratorRange<ConstPairIterator>
{
    return { ConstPairIterator(keyBegin()), ConstPairIterator(keyEnd()) };
}

template<typename T, typename Key, typename Impl>
template<typename TableType>
auto Table<T, Key, Impl>::join(TableType& other)
    -> IteratorRange<JoinIterator<TableType>>
{
    return IteratorRange(
        JoinIterator<TableType>(*this, other),
        JoinIterator<TableType>(
            IteratorRange(keyEnd(), keyEnd()),
            IteratorRange(other.keyEnd(), other.keyEnd())
        )
    );
}

template<typename T, typename Key, typename Impl>
template<typename TableType>
auto Table<T, Key, Impl>::join(const TableType& other) const
    -> IteratorRange<ConstJoinIterator<TableType>>
{
    return IteratorRange(
        ConstJoinIterator(*this, other),
        ConstJoinIterator(
            IteratorRange(keyEnd(), keyEnd()),
            IteratorRange(other.keyEnd(), other.keyEnd())
        )
    );
}

} // namespace componentlib
