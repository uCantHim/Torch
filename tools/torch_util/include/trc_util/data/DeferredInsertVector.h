#pragma once

#include <algorithm>
#include <concepts>
#include <functional>
#include <mutex>
#include <ranges>
#include <vector>

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

        struct GuardedRange
        {
        public:
            auto begin() -> iterator {
                return first;
            }

            auto end() -> iterator {
                return last;
            }

        private:
            friend class DeferredInsertVector<T>;

            GuardedRange(iterator first, iterator last, std::unique_lock<std::mutex> lock)
                : first(std::move(first)), last(std::move(last)),lock(std::move(lock))
            {}

            iterator first;
            iterator last;
            std::unique_lock<std::mutex> lock;
        };

        /**
         * @brief Apply all pending modifications
         */
        void update();

        auto size() const -> size_t;
        bool empty() const;
        auto capacity() const -> size_t;

        void reserve(size_t newCapacity);

        auto at(size_t i) -> T&;
        auto at(size_t i) const -> const T&;
        auto operator[](size_t i) -> T&;
        auto operator[](size_t i) const -> const T&;

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
        auto iter() -> GuardedRange;

    private:
        std::mutex modificationsLock;
        std::vector<T> newItems;
        std::vector<const_iterator> erasedItems;

        std::mutex itemsLock;
        std::vector<T> items;
    };

    template<typename T> requires std::move_constructible<T>
    void DeferredInsertVector<T>::update()
    {
        std::scoped_lock lock(modificationsLock, itemsLock);

        // Erase items
        for (auto it : erasedItems) {
            items.erase(it);
        }

        // Insert new items
        std::move(newItems.begin(), newItems.end(), std::back_inserter(items));

        erasedItems.clear();
        newItems.clear();
    }

    template<typename T> requires std::move_constructible<T>
    auto DeferredInsertVector<T>::size() const -> size_t
    {
        std::scoped_lock lock(itemsLock);
        return items.size();
    }

    template<typename T> requires std::move_constructible<T>
    bool DeferredInsertVector<T>::empty() const
    {
        std::scoped_lock lock(itemsLock);
        return items.empty();
    }

    template<typename T> requires std::move_constructible<T>
    auto DeferredInsertVector<T>::capacity() const -> size_t
    {
        std::scoped_lock lock(itemsLock);
        return items.capacity();
    }

    template<typename T> requires std::move_constructible<T>
    void DeferredInsertVector<T>::reserve(size_t newCapacity)
    {
        std::scoped_lock lock(itemsLock);
        items.reserve(newCapacity);
    }

    template<typename T> requires std::move_constructible<T>
    auto DeferredInsertVector<T>::at(size_t i) -> T&
    {
        std::scoped_lock lock(itemsLock);
        return items.at(i);
    }

    template<typename T> requires std::move_constructible<T>
    auto DeferredInsertVector<T>::at(size_t i) const -> const T&
    {
        std::scoped_lock lock(itemsLock);
        return items.at(i);
    }

    template<typename T> requires std::move_constructible<T>
    auto DeferredInsertVector<T>::operator[](size_t i) -> T&
    {
        std::scoped_lock lock(itemsLock);
        return items[i];
    }

    template<typename T> requires std::move_constructible<T>
    auto DeferredInsertVector<T>::operator[](size_t i) const -> const T&
    {
        std::scoped_lock lock(itemsLock);
        return items[i];
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
        erasedItems.insert(
            std::ranges::upper_bound(erasedItems, pos, std::greater<const_iterator>{}),
            pos
        );
    }

    template<typename T> requires std::move_constructible<T>
    auto DeferredInsertVector<T>::iter() -> GuardedRange
    {
        std::unique_lock lock(itemsLock);
        return { items.begin(), items.end(), std::move(lock) };
    }
} // namespace trc::data
