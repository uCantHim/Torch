#pragma once

#include <algorithm>
#include <concepts>
#include <functional>
#include <mutex>
#include <ranges>
#include <shared_mutex>
#include <vector>

#include "trc_util/algorithm/IteratorRange.h"

namespace trc::data
{
    template<typename T>
        requires std::move_constructible<T>
    class DeferredInsertVector
    {
    public:
        using value_type      = typename std::vector<T>::value_type;
        using reference       = typename std::vector<T>::reference;
        using const_reference = typename std::vector<T>::const_reference;
        using size_type       = typename std::vector<T>::size_type;

        using iterator        = typename std::vector<T>::iterator;
        using const_iterator  = typename std::vector<T>::const_iterator;

        /**
         * @brief Apply all pending modifications
         */
        void update();

        auto size() const -> size_type;
        bool empty() const;

        /**
         * @return bool True if no modifications are pending
         */
        bool none_pending() const;

        auto capacity() const -> size_type;
        void reserve(size_type newCapacity);

        auto at(size_type i) -> reference;
        auto at(size_type i) const -> const_reference;

        /**
         * @brief Construct an element in-place at the end of the container
         *
         * Defer construction and insertion if other blocking operations on
         * the container are currently in progress.
         */
        template<typename ...Args>
        void emplace_back(Args&&... args);

        /**
         * @brief Erase an element from the container
         *
         * Defer the operation if other blocking operations on the
         * container are currently in progress.
         */
        void erase(const_iterator pos);

        /**
         * @brief Iterate over the container's items
         *
         * @return GuardedRange A range. Holds a lock on the container's
         *                      storage for the range object's lifetime.
         */
        auto iter() -> algorithm::GuardedRange<iterator, std::shared_mutex>;

    private:
        mutable std::mutex modificationsLock;
        std::vector<T> newItems;
        std::vector<const_iterator> erasedItems;

        mutable std::shared_mutex itemsLock;
        std::vector<T> items;
    };

    template<typename T> requires std::move_constructible<T>
    void DeferredInsertVector<T>::update()
    {
        std::scoped_lock lock(modificationsLock, itemsLock);

        // Erase items
        std::ranges::unique(erasedItems);
        for (auto it : erasedItems) {
            items.erase(it);
        }

        // Insert new items
        std::move(newItems.begin(), newItems.end(), std::back_inserter(items));

        erasedItems.clear();
        newItems.clear();
    }

    template<typename T> requires std::move_constructible<T>
    auto DeferredInsertVector<T>::size() const -> size_type
    {
        std::shared_lock lock(itemsLock);
        return items.size();
    }

    template<typename T> requires std::move_constructible<T>
    bool DeferredInsertVector<T>::empty() const
    {
        std::shared_lock lock(itemsLock);
        return items.empty();
    }

    template<typename T> requires std::move_constructible<T>
    bool DeferredInsertVector<T>::none_pending() const
    {
        std::scoped_lock lock(modificationsLock);
        return newItems.empty() && erasedItems.empty();
    }

    template<typename T> requires std::move_constructible<T>
    auto DeferredInsertVector<T>::capacity() const -> size_type
    {
        std::shared_lock lock(itemsLock);
        return items.capacity();
    }

    template<typename T> requires std::move_constructible<T>
    void DeferredInsertVector<T>::reserve(size_type newCapacity)
    {
        std::scoped_lock lock(itemsLock);
        items.reserve(newCapacity);
    }

    template<typename T> requires std::move_constructible<T>
    auto DeferredInsertVector<T>::at(size_type i) -> reference
    {
        std::shared_lock lock(itemsLock);
        return items.at(i);
    }

    template<typename T> requires std::move_constructible<T>
    auto DeferredInsertVector<T>::at(size_type i) const -> const_reference
    {
        std::shared_lock lock(itemsLock);
        return items.at(i);
    }

    template<typename T> requires std::move_constructible<T>
    template<typename ...Args>
    void DeferredInsertVector<T>::emplace_back(Args&&... args)
    {
        if (itemsLock.try_lock())
        {
            items.emplace_back(std::forward<Args>(args)...);
            itemsLock.unlock();
            return;
        }

        std::scoped_lock lock(modificationsLock);
        newItems.emplace_back(std::forward<Args>(args)...);
    }

    template<typename T> requires std::move_constructible<T>
    void DeferredInsertVector<T>::erase(const_iterator pos)
    {
        if (itemsLock.try_lock())
        {
            items.erase(pos);
            itemsLock.unlock();
            return;
        }

        std::scoped_lock lock(modificationsLock);

        // Keep list of erased items sorted in reverse order
        erasedItems.insert(
            std::ranges::upper_bound(erasedItems, pos, std::greater<const_iterator>{}),
            pos
        );
    }

    template<typename T> requires std::move_constructible<T>
    auto DeferredInsertVector<T>::iter() -> algorithm::GuardedRange<iterator, std::shared_mutex>
    {
        std::unique_lock lock(itemsLock);
        return { items.begin(), items.end(), std::move(lock) };
    }
} // namespace trc::data
