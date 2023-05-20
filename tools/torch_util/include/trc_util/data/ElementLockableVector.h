#pragma once

#include <cassert>
#include <concepts>
#include <memory>
#include <shared_mutex>
#include <vector>

#include "trc_util/async/AccessGuard.h"

namespace trc::data
{
    constexpr size_t kDefaultSafeChunkStorageChunkSize{ 20 };

    /**
     * @brief A highly restricted container that allows locked access to
     *        individual elements
     *
     * Every stored object is allocated individually on the heap, which
     * makes this container primarily useful for safe access to a limited
     * number of objects.
     */
    template<typename T>
        requires std::is_default_constructible_v<T>
    class ElementLockableVector
    {
    public:
        using value_type = T;
        using reference = T&;
        using const_reference = const T&;
        using pointer = T*;
        using const_pointer = const T*;
        using size_type = size_t;

    private:
        using Element = async::AccessGuard<value_type>;

    public:
        using ReadAccess = typename Element::ReadAccess;
        using WriteAccess = typename Element::WriteAccess;

        auto size() const -> size_type
        {
            return elems.size();
        }

        /**
         * @brief Acquire shared read access for an element
         */
        auto read(size_type index) const -> ReadAccess
        {
            return getElem(index)->read();
        }

        /**
         * @brief Acquire exclusive write access for an element
         */
        auto write(size_type index) -> WriteAccess
        {
            return getElem(index)->modify();
        }

        /**
         * @brief Ensure space a minimum number of elements in the container
         *
         * Existing references to elements will *not* be invalidated if a
         * resize occurs.
         *
         * @param size_type newSize Resize the container to hold at least
         *                          `newSize` elements. Does nothing if the
         *                          current size is greater than `newSize`.
         */
        void resize_to_fit(size_type newSize)
            requires std::is_default_constructible_v<value_type>
        {
            if (newSize > size())
            {
                std::scoped_lock lock(listLock);
                while (newSize > size()) {
                    elems.emplace_back(std::make_shared<Element>());
                }
            }
        }

        /**
         * @brief Destroy all elements in the container
         *
         * This *can* be called without invalidating existing references to
         * elements, although the calling thread will block until all locks on
         * all elements have been released.
         */
        void clear()
        {
            std::vector<std::shared_ptr<Element>> oldElems;

            // Empty the list of elements by moving it
            {
                std::scoped_lock lock(listLock);
                auto oldElems = std::move(elems);
                assert(elems.empty());
            }

            // Now safely deconstruct the elements. The ElementLockableVector
            // remains available for use in other threads during this.

            // First acquire exclusive access to each element once. This is
            // sufficient because the elements are no longer reachable from
            // anywhere else.
            for (auto& el : oldElems) {
                el->modify();
            }

            // All elements get deleted once we go out of scope. No mutex is
            // held anymore.
        }

    private:
        auto getElem(size_type index) -> std::shared_ptr<Element>
        {
            std::shared_lock lock(listLock);
            if (index >= elems.size()) {
                throw std::out_of_range("[In ElementLockableVector::getElem]: Index out of range.");
            }

            return elems.at(index);
        }

        auto getElem(size_type index) const -> std::shared_ptr<const Element>
        {
            std::shared_lock lock(listLock);
            if (index >= elems.size()) {
                throw std::out_of_range("[In ElementLockableVector::getElem]: Index out of range.");
            }

            return elems.at(index);
        }

        mutable std::shared_mutex listLock;

        // These must be shared_ptrs because I can't clear the vector if
        // it stores unique_ptrs to potentially acquired mutexes.
        std::vector<std::shared_ptr<Element>> elems;
    };
} // namespace trc::data
