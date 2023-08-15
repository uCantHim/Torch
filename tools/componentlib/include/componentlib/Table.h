#pragma once

#include <cassert>
#include <limits>
#include <concepts>
#include <vector>
#include <optional>

#include <trc_util/algorithm/IteratorRange.h>
#include <trc_util/functional/Maybe.h>
namespace trc {
    using functional::Maybe;
}

#include "TableIterators.h"
#include "TableJoinIterator.h"

namespace componentlib
{

using trc::algorithm::IteratorRange;

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
template<typename T, TableKey Key>
class Table
{
public:
    using value_type = T;
    using reference = T&;
    using const_reference = const T&;
    using pointer = T*;
    using const_pointer = const T*;

    using size_type = size_t;

    using key_type = Key;

private:
    static constexpr bool stableStorage = requires {
        typename TableTraits<T>::UniqueStorage;
    };
    using StoredType = std::conditional_t<stableStorage, std::unique_ptr<value_type>, value_type>;

public:
    Table() = default;

    /**
     * @return size_t Number of objects in the table
     */
    auto size() const -> size_t;

    /**
     * @brief Access the table's raw data
     *
     * @return pointer A pointer to the address of the first element
     */
    auto data() -> pointer;

    /**
     * @brief Access the table's raw data
     *
     * @return pointer A pointer to the address of the first element
     */
    auto data() const -> const_pointer;

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
    inline auto get(Key key) -> reference;

    /**
     * @param Key key
     *
     * @throw std::out_of_range if no object with key `key` exists in the
     *                          table.
     */
    inline auto get(Key key) const -> const_reference;

    inline auto try_get(Key key) -> std::optional<pointer>;
    inline auto try_get(Key key) const -> std::optional<const_pointer>;

    inline auto get_m(Key key) -> trc::Maybe<reference>;
    inline auto get_m(Key key) const -> trc::Maybe<const_reference>;

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
    inline auto emplace(Key key, Args&&... args) -> reference;

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
    inline auto try_emplace(Key key, Args&&... args) -> std::pair<reference, bool>;

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
    inline auto erase(Key key) -> value_type;

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
    inline auto try_erase(Key key) noexcept -> std::optional<value_type>;

    /**
     * @brief Try to erase an object
     *
     * @param Key key
     *
     * @return trc::Maybe<T> Nothing if no object was erased, the
     *                               erased object otherwise.
     */
    inline auto erase_m(Key key) noexcept -> trc::Maybe<value_type>;

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
     * @param size_t minSize Pre-allocate enough storage for `minSize`
     *                       elements.
     */
    inline void reserve(size_t minSize);

    /**
     * @brief Reserve a minimum amount of storage space for elements and
     *        keys separately
     *
     * @param size_t minSizeElems Pre-allocate enough storage for `minSize`
     *                            elements.
     * @param size_t minSizeKeys Pre-allocate enough storage for `minSize`
     *                           keys.
     */
    inline void reserve(size_t minSizeElems, size_t minSizeKeys);

public:
    // -------------------------- //
    //      Iterator classes      //
    // -------------------------- //

    // iterator types
    using ValueIterator = TableValueIterator<Table<T, Key>>;
    using KeyIterator = TableKeyIterator<Table<T, Key>>;
    using PairIterator = TablePairIterator<Table<T, Key>>;
    template<typename Other>
    using JoinIterator = TableJoinIterator<Table<T, Key>, Other>;

    // const-iterator types
    using ConstValueIterator = const TableValueIterator<const Table<T, Key>>;
    using ConstKeyIterator = const TableKeyIterator<const Table<T, Key>>;
    using ConstPairIterator = const TablePairIterator<const Table<T, Key>>;
    template<typename Other>
    using ConstJoinIterator = const TableJoinIterator<Table<T, Key>, Other>;

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
    inline auto _do_get(IndexType index) -> reference;
    inline auto _do_get(IndexType index) const -> const_reference;
    template<typename ...Args>
    inline auto _do_emplace(IndexType index, Args&&... args) -> reference;
    template<typename ...Args>
    inline auto _do_emplace_back(Args&&... args) -> std::pair<IndexType, reference>;
    inline auto _do_erase_unsafe(Key key) -> value_type;

    std::vector<StoredType> objects;
    std::vector<IndexType> indices;
};

template<typename T, TableKey Key>
template<typename TableType>
auto Table<T, Key>::join(TableType& other)
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

template<typename T, TableKey Key>
template<typename TableType>
auto Table<T, Key>::join(const TableType& other) const
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
