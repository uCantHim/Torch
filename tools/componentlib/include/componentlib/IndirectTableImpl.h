#pragma once

#include <algorithm>
#include <concepts>
#include <limits>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

#include "IndirectTableImplIterators.h"
#include "TableBase.h"

namespace componentlib
{
    template<typename T, TableKey Key>
    class IndirectTableImpl
    {
    public:
        using value_type = T;
        using reference = T&;
        using const_reference = const T&;
        using pointer = T*;
        using const_pointer = const T*;
        using key_type = Key;

        using size_type = std::size_t;

        void reserve(size_type minElems)
        {
            objects.reserve(minElems);
            indices.reserve(minElems);
        }

        bool contains(key_type key) const {
            return indices.size() > static_cast<size_t>(key)
                && indices.at(static_cast<size_t>(key)) != NONE;
        }

        auto at(key_type key) -> pointer
        {
            const size_type index = _index_at_key(key);
            if (index == NONE) {
                return nullptr;
            }
            return &_do_get(index);
        }

        auto at(key_type key) const -> const_pointer
        {
            const size_type index = _index_at_key(key);
            if (index == NONE) {
                return nullptr;
            }
            return &_do_get(index);
        }

        /**
         * Construct and overwrite. Expand space if key is new.
         */
        template<typename ...Args>
        auto emplace(key_type key, Args&&... args) -> reference
        {
            if (static_cast<size_t>(key) >= indices.size()) {
                indices.resize(static_cast<size_t>(key) + 1, NONE);
            }

            const size_type index = _index_at_key(key);
            if (index != NONE) {
                return _do_emplace(index, std::forward<Args>(args)...);
            }

            auto [newIndex, obj] = _do_emplace_back(std::forward<Args>(args)...);
            indices.at(static_cast<size_type>(key)) = newIndex;
            return obj;
        }

        /**
         * Try to erase.
         */
        bool erase(key_type key)
        {
            if (contains(key)) {
                _do_erase_unsafe(key);
            }
            return false;
        }

        void clear()
        {
            objects.clear();
            indices.clear();
        }

        // iterator types
        using ValueIterator = IndirectTableValueIterator<IndirectTableImpl<T, Key>>;
        using KeyIterator = IndirectTableKeyIterator<IndirectTableImpl<T, Key>>;

        // const-iterator types
        using ConstValueIterator = IndirectTableValueIterator<const IndirectTableImpl<T, Key>>;
        using ConstKeyIterator = IndirectTableKeyIterator<const IndirectTableImpl<T, Key>>;

        friend ValueIterator;
        friend ConstValueIterator;
        friend KeyIterator;
        friend ConstKeyIterator;

        auto valueBegin()       -> ValueIterator      { return ValueIterator(objects.begin()); }
        auto valueBegin() const -> ConstValueIterator { return ConstValueIterator(objects.begin()); }
        auto valueEnd()         -> ValueIterator      { return ValueIterator(objects.end()); }
        auto valueEnd()   const -> ConstValueIterator { return ConstValueIterator(objects.end()); }

        auto keyBegin()       -> KeyIterator      { return KeyIterator(indices.begin(), *this); }
        auto keyBegin() const -> ConstKeyIterator { return ConstKeyIterator(indices.begin(), *this); }
        auto keyEnd()         -> KeyIterator      { return KeyIterator(indices.end(), *this); }
        auto keyEnd()   const -> ConstKeyIterator { return ConstKeyIterator(indices.end(), *this); }

    private:
        /** Indirection index that indicates that a key does not exist */
        static constexpr size_type NONE = std::numeric_limits<size_type>::max();

        auto _index_at_key(key_type key) const -> size_type;
        auto _do_get(size_type index) -> reference;
        auto _do_get(size_type index) const -> const_reference;
        template<typename ...Args>
        auto _do_emplace(size_type index, Args&&... args) -> reference;
        template<typename ...Args>
        auto _do_emplace_back(Args&&... args) -> std::pair<size_type, reference>;
        auto _do_erase_unsafe(key_type key) -> value_type;

        std::vector<value_type> objects;
        std::vector<size_type> indices;
    };



    template<typename T, TableKey Key>
    inline auto IndirectTableImpl<T, Key>::_index_at_key(key_type key) const -> size_type
    {
        return indices.at(static_cast<size_type>(key));
    }

    template<typename T, TableKey Key>
    inline auto IndirectTableImpl<T, Key>::_do_get(size_type index) -> reference
    {
        return objects.at(index);
    }

    template<typename T, TableKey Key>
    inline auto IndirectTableImpl<T, Key>::_do_get(size_type index) const -> const_reference
    {
        return objects.at(index);
    }

    template<typename T, TableKey Key>
    template<typename ...Args>
    inline auto IndirectTableImpl<T, Key>::_do_emplace(size_type index, Args&&... args) -> reference
    {
        return *objects.emplace(
            objects.begin() + index,
            std::forward<Args>(args)...
        );
    }

    template<typename T, TableKey Key>
    template<typename ...Args>
    inline auto IndirectTableImpl<T, Key>::_do_emplace_back(Args&&... args)
        -> std::pair<size_type, reference>
    {
        return {
            objects.size(),
            objects.emplace_back(std::forward<Args>(args)...)
        };
    }

    template<typename T, TableKey Key>
    inline auto IndirectTableImpl<T, Key>::_do_erase_unsafe(key_type key) -> value_type
    {
        size_type& index = indices.at(static_cast<size_type>(key));

        // Unstably remove object
        std::swap(objects.at(index), objects.back());
        value_type result = std::move(objects.back());
        objects.pop_back();

        // Flag object as not existing and modify index that pointed at the moved object
        auto swappedObjectIndexIt = std::find(indices.begin(), indices.end(), objects.size());
        if (swappedObjectIndexIt != indices.end()) {
            *swappedObjectIndexIt = index;
        }
        index = NONE;

        return result;
    }
} // namespace componentlib
