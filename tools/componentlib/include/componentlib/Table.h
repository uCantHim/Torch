#pragma once

#include <cassert>
#include <limits>
#include <concepts>
#include <vector>
#include <optional>

#include <trc_util/functional/Maybe.h>
namespace trc {
    using functional::Maybe;
}

#include "TableIterators.h"
#include "TableJoinIterator.h"
#include "IteratorRange.h"

namespace componentlib
{

/**
 * @brief Specializable struct that can declare options for table contents
 *
 * Options:
 *
 *  - Declare a type `UniqueStorage` in your specialization to store
 *    elements as unique_ptrs (instead of value types) in the table.
 */
template<typename T>
struct TableTraits
{
    // struct UniqueStorage {};
};

/**
 * Efficiently iterable, contiguous, index-based, database-style table.
 *
 * Manipulating operations may change memory addresses of elements. If you
 * want to take permanent pointers to objects stored in the table, declare a
 * type `UniqueStorage` in a specialized template instance of `TableTraits`
 * with your type T. This will cause instances of `Table<T>` to store their
 * elements as `std::unique_ptr<T>` instead of plain `T`s.
 *
 * Example:
 *
 *     template<>
 *     struct TableTraits<YourType>
 *     {
 *         struct UniqueStorage {};
 *     };
 *
 * Of course, this will probably trade memory contiguousness for convenient
 * usage.
 */
template<typename Component, TableKey Key>
class Table
{
public:
    using value_type = Component;
    using reference = value_type&;
    using pointer = value_type*;
    using key_type = Key;

private:
    static constexpr bool stableStorage = requires {
        typename TableTraits<Component>::UniqueStorage;
    };
    using StoredType = std::conditional_t<stableStorage, std::unique_ptr<value_type>, value_type>;

public:
    Table() = default;

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
    inline bool has(Key key) const;

    /**
     * @param Key key
     *
     * @throw std::out_of_range if no object with key `key` exists in the
     *                          table.
     */
    inline auto get(Key key) -> Component&;

    /**
     * @param Key key
     *
     * @throw std::out_of_range if no object with key `key` exists in the
     *                          table.
     */
    inline auto get(Key key) const -> const Component&;

    inline auto try_get(Key key) -> std::optional<Component*>;
    inline auto try_get(Key key) const -> std::optional<const Component*>;

    inline auto get_m(Key key) -> trc::Maybe<Component&>;
    inline auto get_m(Key key) const -> trc::Maybe<const Component&>;

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
        requires std::constructible_from<Component, Args&&...>
    inline auto emplace(Key key, Args&&... args) -> Component&;

    /**
     * @brief Try to insert an object at a specific key
     *
     * Constructs the object in-place. Does not overwrite existing objects
     * at `key`.
     *
     * @return std::pair<Component&, bool> The new object and `true` if the
     *         insertion was successful. The existing object and `false`
     *         otherwise.
     */
    template<typename ...Args>
        requires std::constructible_from<Component, Args&&...>
    inline auto try_emplace(Key key, Args&&... args) -> std::pair<Component&, bool>;

public:
    // ------------------------- //
    //      Element erasure      //
    // ------------------------- //

    /**
     * @brief Throws if no object exists at key `key`
     *
     * @param Key key
     *
     * @return Component The erased object at key `key`.
     * @throw std::out_of_range if no object exists at key `key`
     */
    inline auto erase(Key key) -> Component;

    /**
     * @brief Try to erase an object
     *
     * @param Key key
     *
     * Does nothing if no object with the specified key exists.
     *
     * @return std::optional<Component> The erased Component if an object
     *                                  was erased, nullopt otherwise.
     */
    inline auto try_erase(Key key) noexcept -> std::optional<Component>;

    /**
     * @brief Try to erase an object
     *
     * @param Key key
     *
     * @return trc::Maybe<Component> Nothing if no object was erased, the
     *                               erased object otherwise.
     */
    inline auto erase_m(Key key) noexcept -> trc::Maybe<Component>;

    /**
     * @brief Erase all objects and keys from the table
     */
    inline void clear() noexcept;

public:
    // -------------------------- //
    //      Iterator classes      //
    // -------------------------- //

    // iterator types
    using ValueIterator = TableValueIterator<Table<Component, Key>>;
    using KeyIterator = TableKeyIterator<Table<Component, Key>>;
    using PairIterator = TablePairIterator<Table<Component, Key>>;
    template<typename Other>
    using JoinIterator = TableJoinIterator<Table<Component, Key>, Other>;

    // const-iterator types
    using ConstValueIterator = const TableValueIterator<const Table<Component, Key>>;
    using ConstKeyIterator = const TableKeyIterator<const Table<Component, Key>>;
    using ConstPairIterator = const TablePairIterator<const Table<Component, Key>>;
    template<typename Other>
    using ConstJoinIterator = const TableJoinIterator<Table<Component, Key>, Other>;

    friend ValueIterator;
    friend ConstValueIterator;
    friend KeyIterator;
    friend ConstKeyIterator;

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
    using IndexType = size_t;
    static constexpr IndexType NONE = std::numeric_limits<IndexType>::max();

    inline auto _index_at_key(Key key) -> IndexType&;
    inline auto _index_at_key(Key key) const -> const IndexType&;
    inline auto _do_get(IndexType index) -> Component&;
    inline auto _do_get(IndexType index) const -> const Component&;
    template<typename ...Args>
    inline auto _do_emplace(IndexType index, Args&&... args) -> Component&;
    template<typename ...Args>
    inline auto _do_emplace_back(Args&&... args) -> std::pair<IndexType, Component&>;
    inline auto _do_erase_unsafe(Key key) -> Component;

    std::vector<StoredType> objects;
    std::vector<IndexType> indices;
};

template<typename Component, TableKey Key>
template<typename TableType>
auto Table<Component, Key>::join(TableType& other)
    -> IteratorRange<JoinIterator<TableType>>
{
    return IteratorRange(
        JoinIterator(*this, other),
        JoinIterator(
            IteratorRange(keyEnd(), keyEnd()),
            IteratorRange(other.keyEnd(), other.keyEnd())
        )
    );
}

template<typename Component, TableKey Key>
template<typename TableType>
auto Table<Component, Key>::join(const TableType& other) const
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


#include "Table.inl"
#include "TableInternal.inl"

} // namespace componentlib
