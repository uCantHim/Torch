#pragma once

#include <cstdint>
#include <memory>

#include <trc_util/data/IndexMap.h>
#include <trc_util/data/OptionalStorage.h>

#include "TableBase.h"
#include "StableTableImplIterators.h"

namespace componentlib
{
    /**
     * @brief Table with stable addresses to elements
     *
     * Pool-like implementation of a table that keeps elements at stable
     * addresses by managing memory in persistent chunks. Slower sequential
     * iteration over values than unstable implementations such as
     * IndirectTable.
     *
     * Useful for concurrent work *if* space (i.e. chunk memory) is allocated
     * beforehand via `reserve`.
     */
    template<typename T, TableKey Key>
    class StableTableImpl
    {
    public:
        using value_type = T;
        using reference = T&;
        using const_reference = const T&;
        using pointer = T*;
        using const_pointer = const T*;
        using key_type = Key;

        using size_type = std::size_t;

        void reserve(size_type minElems);

        bool contains(key_type key) const;

        auto at(key_type key) -> pointer;
        auto at(key_type key) const -> const_pointer;

        /**
         * Construct and overwrite. Expand space if key is new.
         */
        template<typename ...Args>
        auto emplace(key_type key, Args&&... args) -> reference;

        /**
         * Try to erase.
         */
        bool erase(key_type key);

        void clear();

        // iterator types
        using ValueIterator = StableTableValueIterator<StableTableImpl<T, Key>>;
        using KeyIterator = StableTableKeyIterator<StableTableImpl<T, Key>>;

        // const-iterator types
        using ConstValueIterator = StableTableValueIterator<const StableTableImpl<T, Key>>;
        using ConstKeyIterator = StableTableKeyIterator<const StableTableImpl<T, Key>>;

        auto valueBegin() -> ValueIterator {
            return ValueIterator(*this, key_type(0));
        }
        auto valueBegin() const -> ConstValueIterator {
            return ConstValueIterator(*this, key_type(0));
        }
        auto valueEnd() -> ValueIterator {
            return ValueIterator(*this, key_type(chunks.size() * kChunkSize));
        }
        auto valueEnd() const -> ConstValueIterator {
            return ConstValueIterator(*this, key_type(chunks.size() * kChunkSize));
        }

        auto keyBegin() -> KeyIterator {
            return KeyIterator(*this, key_type(0));
        }
        auto keyBegin() const -> ConstKeyIterator {
            return ConstKeyIterator(*this, key_type(0));
        }
        auto keyEnd() -> KeyIterator {
            return KeyIterator(*this, key_type(chunks.size() * kChunkSize));
        }
        auto keyEnd() const -> ConstKeyIterator {
            return ConstKeyIterator(*this, key_type(chunks.size() * kChunkSize));
        }

    private:
        template<typename> friend class StableTableIterator;  // Friend the base for all iters
        friend ValueIterator;
        friend ConstValueIterator;
        friend KeyIterator;
        friend ConstKeyIterator;

        static_assert(requires (StableTableImpl<T, Key> t) {
            { *std::declval<ValueIterator>() }      -> std::same_as<reference>;
            { *std::declval<ConstValueIterator>() } -> std::same_as<const_reference>;
            { *std::declval<KeyIterator>() }        -> std::same_as<const key_type&>;
            { *std::declval<ConstKeyIterator>() }   -> std::same_as<const key_type&>;
            { std::declval<ValueIterator>().operator->() }      -> std::same_as<pointer>;
            { std::declval<ConstValueIterator>().operator->() } -> std::same_as<const_pointer>;
            { std::declval<KeyIterator>().operator->() }        -> std::same_as<const key_type*>;
            { std::declval<ConstKeyIterator>().operator->() }   -> std::same_as<const key_type*>;
        });

        static constexpr size_type kChunkSize = 40;

        static constexpr auto chunkIndex(key_type index) -> size_type {
            return static_cast<size_type>(index) / kChunkSize;
        }
        static constexpr auto elemIndex(key_type index) -> size_type {
            return static_cast<size_type>(index) % kChunkSize;
        }

        using Chunk = trc::data::OptionalStorage<value_type, kChunkSize>;

        trc::data::IndexMap<size_type, std::unique_ptr<Chunk>> chunks;
    };



    template<typename T, TableKey Key>
    void StableTableImpl<T, Key>::reserve(size_type minElems)
    {
        const size_type size = (minElems / kChunkSize) + 1;
        for (size_type i = chunks.size(); i < size; ++i) {
            chunks.emplace(i, std::make_unique<Chunk>());
        }
    }

    template<typename T, TableKey Key>
    bool StableTableImpl<T, Key>::contains(key_type key) const
    {
        const size_type chunk = chunkIndex(key);
        const size_type elem = elemIndex(key);
        return chunks.size() > chunk && chunks.at(chunk)->valid(elem);
    }

    template<typename T, TableKey Key>
    auto StableTableImpl<T, Key>::at(key_type key) -> pointer
    {
        if (!contains(key)) {
            return nullptr;
        }
        return &chunks.at(chunkIndex(key))->at(elemIndex(key));
    }

    template<typename T, TableKey Key>
    auto StableTableImpl<T, Key>::at(key_type key) const -> const_pointer
    {
        if (!contains(key)) {
            return nullptr;
        }
        return &chunks.at(chunkIndex(key))->at(elemIndex(key));
    }

    template<typename T, TableKey Key>
    template<typename ...Args>
    auto StableTableImpl<T, Key>::emplace(key_type key, Args&&... args) -> reference
    {
        reserve(key);

        const size_type chunk = chunkIndex(key);
        const size_type elem = elemIndex(key);
        return chunks.at(chunk)->emplace(elem, std::forward<Args>(args)...);
    }

    template<typename T, TableKey Key>
    bool StableTableImpl<T, Key>::erase(key_type key)
    {
        if (contains(key))
        {
            const size_type chunk = chunkIndex(key);
            const size_type elem = elemIndex(key);
            return chunks.at(chunk)->erase(elem);
        }

        return false;
    }

    template<typename T, TableKey Key>
    void StableTableImpl<T, Key>::clear()
    {
        chunks = {};
    }
} // namespace componentlib
