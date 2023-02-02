#pragma once

#include <cassert>
#include <cstdint>
#include <array>
#include <bitset>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <vector>

#include "OptionalStorage.h"
#include "ElementLockableVector.h"

namespace trc::util
{
    /**
     * Default chunk size for a `SafeVector<>` instantiation.
     */
    constexpr size_t kSafeVectorDefaultChunkSize{ 40 };

    /**
     * @brief A memory- and thread-safe container
     *
     * 'Memory-safe' because pointers to elements in the vector are never
     * invalidated.
     *
     * 'Thread-safe' because all operations on the vector are atomic so
     * that multiple threads can execute both read AND write operations
     * concurrently. However, modifications to the elements themselves
     * must still be externally synchronized.
     *
     * Memory is organized in chunks. Once a chunk is allocated, it will
     * live forever. The application should use the vector in a way that
     * its space is populated as densely as possible (for example, use
     * trc::data::IdPool).
     *
     * Chunk size can be controlled via the template parameter `ChunkSize`.
     *
     * TODO: Implementing an iterator for SafeVector is very possible.
     */
    template<
        typename T,
        size_t ChunkSize = kSafeVectorDefaultChunkSize
        >
    class SafeVector
    {
    public:
        using value_type = T;
        using pointer = T*;
        using const_pointer = const T*;
        using reference = T&;
        using const_reference = const T&;
        using size_type = size_t;

    private:
        static constexpr inline auto _chunk_index(size_type index) -> size_type {
            return index / ChunkSize;
        }

        static constexpr inline auto _elem_index(size_type index) -> size_type {
            return index % ChunkSize;
        }

    public:
        /**
         * @brief Test whether an element exists at a specific index
         *
         * @return bool
         */
        bool contains(size_type index) const
        {
            const size_type chunk = _chunk_index(index);
            return chunks.size() > chunk
                && chunks.read(chunk)->valid(_elem_index(index));
        }

        /**
         * @brief Retrieve an element at a specific index
         *
         * @throw std::out_of_range if no element exists at index `index`.
         */
        auto at(size_type index) -> reference
        {
            auto chunk = chunks.write(_chunk_index(index));
            return chunk->at(_elem_index(index));
        }

        /**
         * @brief Retrieve an element at a specific index
         *
         * @throw std::out_of_range if no element exists at index `index`.
         */
        auto at(size_type index) const -> const_reference
        {
            auto chunk = chunks.read(_chunk_index(index));
            return chunk->at(_elem_index(index));
        }

        /**
         * @brief Construct an element in-place
         *
         * If an element already exists at `index`, the existing element is
         * destroyed and replaced with the new element.
         *
         * @tparam ...Args `T` must be constructible from a list of values
         *                 of respective types `Args...`.
         *
         * @param size_type index The index at which the new element is
         *                        constructed
         * @param Args...   args  Arguments to the new element's constructor.
         *
         * @return reference A reference to the newly constructed object.
         */
        template<typename ...Args>
            requires std::constructible_from<value_type, Args...>
        auto emplace(size_type index, Args&&... args) -> reference
        {
            const size_type chunk = _chunk_index(index);
            chunks.resize_to_fit(chunk + 1);

            return chunks.write(chunk)->emplace(_elem_index(index), std::forward<Args>(args)...);
        }

        /**
         * @brief Delete an object at a specific index
         *
         * @throw std::out_of_range if no element exists at index `index`.
         */
        void erase(size_type index)
        {
            chunks.write(_chunk_index(index))->erase(_elem_index(index));
        }

        /**
         * @brief Allocate a minimum amount of storage
         *
         * Allocates enough storage to hold at least `new_size` elements.
         * In other words, this function ensures that at least
         * `upper(new_size / chunk_size)` chunks are allocated.
         *
         * @param size_type new_size
         */
        void reserve(size_type new_size)
        {
            chunks.resize_to_fit((new_size + ChunkSize - 1) / ChunkSize);
        }

        /**
         * @brief Destroy all elements and deallocate all storage space
         */
        void clear()
        {
            chunks.clear();
        }

    private:
        using Chunk = data::OptionalStorage<value_type, ChunkSize>;

        /**
         * We need to be able to lock single chunks to ensure atomicity of
         * operations like `emplace` or `erase`.
         */
        data::ElementLockableVector<Chunk> chunks;
    };
} // namespace trc::util
