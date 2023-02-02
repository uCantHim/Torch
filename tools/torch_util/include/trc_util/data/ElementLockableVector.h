#pragma once

#include <concepts>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <vector>

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
        struct Element;

    public:
        using value_type = T;
        using reference = T&;
        using const_reference = const T&;
        using pointer = T*;
        using const_pointer = const T*;
        using size_type = size_t;

        struct ReadAccess
        {
            auto operator*() const & -> const_reference { return elem->value; }
            auto operator->() const -> const_pointer { return &elem->value; }

        private:
            friend ElementLockableVector;
            ReadAccess(std::shared_ptr<const Element> elem)
                : lock(elem->mutex), elem(std::move(elem))
            {}

            std::shared_lock<std::shared_mutex> lock;
            std::shared_ptr<const Element> elem;
        };

        struct WriteAccess
        {
            auto operator*() & -> reference { return elem->value; }
            auto operator->() -> pointer { return &elem->value; }

        private:
            friend ElementLockableVector;
            WriteAccess(std::shared_ptr<Element> elem)
                : lock(elem->mutex), elem(std::move(elem))
            {}

            std::unique_lock<std::shared_mutex> lock;
            std::shared_ptr<Element> elem;
        };

        auto size() const -> size_type
        {
            return elems.size();
        }

        /**
         * @brief Acquire shared read access for an element
         */
        auto read(size_type index) const -> ReadAccess
        {
            return { getElem(index) };
        }

        /**
         * @brief Acquire exclusive write access for an element
         */
        auto write(size_type index) -> WriteAccess
        {
            return { getElem(index) };
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
         * elements. These elements will be deleted once all locks on them
         * are released.
         */
        void clear()
        {
            std::scoped_lock lock(listLock);
            elems.clear();
        }

    private:
        struct Element
        {
            mutable std::shared_mutex mutex;
            value_type value;
        };

        auto getElem(size_type index) -> std::shared_ptr<Element>
        {
            std::shared_lock lock(listLock);
            if (index >= elems.size()) {
                throw std::out_of_range("");
            }

            return elems.at(index);
        }

        auto getElem(size_type index) const -> std::shared_ptr<const Element>
        {
            std::shared_lock lock(listLock);
            if (index >= elems.size()) {
                throw std::out_of_range("");
            }

            return elems.at(index);
        }

        mutable std::shared_mutex listLock;

        // These must be shared_ptrs because I can't clear the vector if
        // it stores unique_ptrs to potentially acquired mutexes.
        std::vector<std::shared_ptr<Element>> elems;
    };
} // namespace trc::data
