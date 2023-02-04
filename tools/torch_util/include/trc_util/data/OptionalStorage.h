#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>

#include <array>
#include <bitset>
#include <functional>
#include <stdexcept>
#include <string>

#include "trc_util/Exception.h"

namespace trc::data
{
    /**
     * Exception type thrown by OptionalStorage when an invalid element is
     * accessed.
     */
    class InvalidElementAccess : public Exception
    {
    public:
        InvalidElementAccess() : Exception("InvalidElementAccess") {}
        InvalidElementAccess(std::string msg)
            : Exception("InvalidElementAccess: " + std::move(msg))
        {}
    };

    /**
     * @brief A fixed-size container that stores nullable objects.
     *
     * Elements are stored contiguously in memory. References to elements
     * are never invalidated.
     *
     * Checks bounds at every access in an if-statement and throws in case
     * of an out-of- bounds access.
     * #define TRC_DISABLE_OPTIONAL_STORAGE_OUT_OF_BOUNDS_EXCEPTIONS to
     * disable exceptions and only check via assertion instead.
     */
    template<typename T, size_t N>
    struct OptionalStorage
    {
        using value_type = T;
        using pointer = T*;
        using const_pointer = const T*;
        using reference = T&;
        using const_reference = const T&;
        using size_type = size_t;

        static constexpr size_type kSize = N;

        OptionalStorage(const OptionalStorage&);
        OptionalStorage(OptionalStorage&&) noexcept;
        OptionalStorage& operator=(const OptionalStorage&);
        OptionalStorage& operator=(OptionalStorage&&) noexcept;

        OptionalStorage() = default;
        ~OptionalStorage() noexcept;

        /**
         * @brief Check whether an element is valid
         *
         * @return bool True if the element at `index` is a valid element.
         * @throw std::out_of_range if `index` is out of bounds and bounds
         *                          checking is enabled.
         */
        inline bool valid(size_type index) const
        {
            checkBounds(index);
            return validBits[index];
        }

        /**
         * @return size_type Size of the storage
         */
        inline auto size() const noexcept -> size_type {
            return kSize;
        }

        /**
         * @brief Unchecked access
         *
         * Is neither bounds checked, nor validity checked.
         *
         * Accessing an invalid element can return a reference to undefined
         * memory, or a reference to an object that has already been
         * deleted.
         */
        inline auto accessUnsafe(size_type index) noexcept -> reference
        {
            return reinterpret_cast<pointer>(data.data())[index];
        }

        /**
         * @brief Unchecked access
         *
         * Is neither bounds checked, nor validity checked.
         *
         * Accessing an invalid element can return a reference to undefined
         * memory, or a reference to an object that has already been
         * deleted.
         */
        inline auto accessUnchecked(size_type index) const noexcept -> const_reference
        {
            return reinterpret_cast<const_pointer>(data.data())[index];
        }

        /**
         * @brief Access an element with validity check
         *
         * @return reference
         * @throw InvalidElementAccess if element at `index` is not valid.
         * @throw std::out_of_range if `index` is out of bounds and bound
         *                          checking is enabled.
         */
        inline auto at(size_type index) -> reference
        {
            checkBounds(index);
            if (!valid(index)) {
                throw InvalidElementAccess("[OptionalStorage::at]: Access to invalid element.");
            }

            return reinterpret_cast<pointer>(data.data())[index];
        }

        /**
         * @brief Access an element with validity check
         *
         * @return const_reference
         * @throw InvalidElementAccess if element at `index` is not valid.
         * @throw std::out_of_range if `index` is out of bounds and bound
         *                          checking is enabled.
         */
        inline auto at(size_type index) const -> const_reference
        {
            checkBounds(index);
            if (!valid(index)) {
                throw InvalidElementAccess("[OptionalStorage::at]: Access to invalid element.");
            }

            return reinterpret_cast<const_pointer>(data.data())[index];
        }

        /**
         * @brief Construct an element in-place, overwriting existing elements
         *
         * Destroys any existing element at `index`.
         *
         * @throw std::out_of_range if `index` is out of bounds and bound
         *                          checking is enabled.
         */
        template<typename ...Args>
        inline auto emplace(size_type index, Args&&... args) -> reference
        {
            checkBounds(index);
            if (valid(index)) {
                destroyElem(index);
            }

            std::byte* dataPtr = data.data() + index * sizeof(value_type);
            new (dataPtr) value_type(std::forward<Args>(args)...);
            validBits[index] = true;

            return *reinterpret_cast<pointer>(dataPtr);
        }

        /**
         * @brief Construct an element in-place if no element is at index
         *
         * Only constructs the new element if there is no valid element
         * already at `index`.
         *
         * @param size_type index The index at which to construct the
         *                        element.
         * @param Args&&... args Arguments to the new object's constructor.
         *
         * @return pair Either the newly constructed element if insertion
         *              was successful, or the existing element otherwise,
         *              and a flag indicating whether the insertion took
         *              place.
         * @throw std::out_of_range if `index` is out of bounds and bound
         *                          checking is enabled.
         */
        template<typename ...Args>
        inline auto try_emplace(size_type index, Args&&... args)
            -> std::pair<std::reference_wrapper<value_type>, bool>
        {
            checkBounds(index);
            if (valid(index)) {
                return { at(index), false };
            }
            return { emplace(index, std::forward<Args>(args)...), true };
        }

        /**
         * @brief Destroy an element and set its status to invalid
         *
         * Does nothing if no element exists at `index`.
         *
         * @param size_type index The index of the element to erase.
         *
         * @return bool True if an element was erased, false otherwise.
         * @throw std::out_of_range if `index` is out of bounds and bound
         *                          checking is enabled.
         */
        inline bool erase(size_type index)
        {
            checkBounds(index);
            if (valid(index))
            {
                destroyElem(index);
                validBits[index] = false;
                return true;
            }
            return false;
        }

        inline void clear()
        {
            for (size_type i = 0; i < kSize; ++i) {
                erase(i);
            }
        }

    private:
        static void copyValidElements(OptionalStorage& dst, const OptionalStorage& src);
        static void moveValidElements(OptionalStorage& dst, OptionalStorage&& src) noexcept;

        inline void checkBounds(size_type index) const
        {
#ifndef TRC_DISABLE_OPTIONAL_STORAGE_OUT_OF_BOUNDS_EXCEPTIONS
            if (index >= kSize)
            {
                throw std::out_of_range("[OptionalStorage::checkBounds]: Tried to access element "
                                        + std::to_string(index) + " in container of size "
                                        + std::to_string(kSize) + ".");
            }
#else
            assert(index < kSize && "OptionalStorage: Tried to access out-of-bounds index.");
#endif
        }

        inline void destroyElem(size_type index) {
            at(index).~value_type();
        }

        std::bitset<kSize> validBits{ 0 };
        std::array<std::byte, kSize * sizeof(value_type)> data{ std::byte{0} };
    };



    template<typename T, size_t N>
    OptionalStorage<T, N>::OptionalStorage(const OptionalStorage& other)
        :
        validBits(other.validBits)
    {
        copyValidElements(*this, other);
    }

    template<typename T, size_t N>
    OptionalStorage<T, N>::OptionalStorage(OptionalStorage&& other) noexcept
        :
        validBits(other.validBits)
    {
        moveValidElements(*this, std::move(other));
    }

    template<typename T, size_t N>
    auto OptionalStorage<T, N>::operator=(const OptionalStorage& other) -> OptionalStorage&
    {
        if (&other != this)
        {
            validBits = other.validBits;
            copyValidElements(*this, other);
        }
        return *this;
    }

    template<typename T, size_t N>
    auto OptionalStorage<T, N>::operator=(OptionalStorage&& other) noexcept -> OptionalStorage&
    {
        if (&other != this)
        {
            validBits = other.validBits;
            moveValidElements(*this, std::move(other));
        }
        return *this;
    }

    template<typename T, size_t N>
    OptionalStorage<T, N>::~OptionalStorage() noexcept
    {
        for (size_type i = 0; i < kSize; ++i)
        {
            try {
                erase(i);
            }
            catch (...) {}
        }
    }

    template<typename T, size_t N>
    void OptionalStorage<T, N>::copyValidElements(OptionalStorage& dst, const OptionalStorage& src)
    {
        for (size_type i = 0; i < kSize; ++i)
        {
            if (src.valid(i)) {
                dst.emplace(i, src.at(i));
            }
            else if (dst.valid(i)) {
                dst.erase(i);
            }
        }
    }

    template<typename T, size_t N>
    void OptionalStorage<T, N>::moveValidElements(
        OptionalStorage& dst,
        OptionalStorage&& src) noexcept
    {
        for (size_type i = 0; i < kSize; ++i)
        {
            try {
                if (src.valid(i))
                {
                    dst.emplace(i, std::move(src.at(i)));
                    src.erase(i);
                }
                else if (dst.valid(i)) {
                    dst.erase(i);
                }
            }
            catch (...) {
                // This should never be able to throw, but try/catch just in case
                try { dst.erase(i); } catch (...) {}
            }
        }
    }
} // namespace trc::data
