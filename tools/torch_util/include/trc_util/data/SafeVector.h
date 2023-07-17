#pragma once

#include <cassert>
#include <cstdint>

#include <concepts>
#include <optional>
#include <stdexcept>
#include <utility>

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
     * 'Thread-safe' because all operations on the vector are atomic so that
     * multiple threads can execute both read AND write operations concurrently.
     * However, modifications to the elements themselves must still be
     * externally synchronized.
     *
     * Provides additional functions to apply complex operations atomically to
     * elements via `copyAtomically` and `applyAtomically`. Beware that all bets
     * are off once you use raw pointers to elements (such as those returned
     * from `at` or from iterators).
     *
     * Memory is organized in chunks. Once a chunk is allocated it will live
     * forever, or until `clear` is called on the vector. The application should
     * use the vector in a way that its space is populated as densely as
     * possible (for example, use trc::data::IdPool).
     *
     * Chunk size can be controlled via the template parameter `ChunkSize`.
     * Smaller chunks mean:
     *  - Less locking contention. When accessing single elements, the vector
     *    locks the entire chunk containing that element, so smaller chunks
     *    should help reduce wait-overhead for frequent concurrent accesses.
     *  - Iterating over the vector becomes less cache-efficient.
     *  - Higher ratio of memory overhead from management structures to actual
     *    data. For example, we need one mutex per chunk.
     */
    template<
        typename T,
        size_t ChunkSize = kSafeVectorDefaultChunkSize
        >
    class SafeVector
    {
    public:
        template<bool Constant>
        class Iterator;

        using value_type = T;
        using pointer = T*;
        using const_pointer = const T*;
        using reference = T&;
        using const_reference = const T&;
        using size_type = size_t;

        /**
         * The iterator does not hold permanent locks, so acting upon the
         * `SafeVector` while iterating over it will not deadlock.
         */
        using iterator = Iterator<false>;
        using const_iterator = Iterator<true>;

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
         * This is unsafe in that the object pointed to by the returned
         * reference may be accessed by multiple threads concurrently. The
         * pointed-to memory will never be deallocated, but data races on its
         * contents will occur. For atomic read/write operations on individual
         * elements, use `SafeVector<>::copyAtomically` or
         * `SafeVector<>::applyAtomically`.
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
         * This is unsafe in that the object pointed to by the returned
         * reference may be accessed by multiple threads concurrently. The
         * pointed-to memory will never be deallocated, but data races on its
         * contents will occur. For atomic read/write operations on individual
         * elements, use `SafeVector<>::copyAtomically` or
         * `SafeVector<>::applyAtomically`.
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
         * @brief Try to construct an element in-place
         *
         * If an element already exists at `index`, no element is constructed.
         *
         * @tparam ...Args `T` must be constructible from a list of values
         *                 of respective types `Args...`.
         *
         * @param size_type index The index at which the new element is
         *                        constructed
         * @param Args...   args  Arguments to the new element's constructor.
         *
         * @return pair<reference, bool> A reference to the element at `index`
         *                               and a flag indicating whether insertion
         *                               has taken place.
         */
        template<typename ...Args>
            requires std::constructible_from<value_type, Args...>
        auto try_emplace(size_type index, Args&&... args)
            -> std::pair<std::reference_wrapper<value_type>, bool>
        {
            const size_type chunkIndex = _chunk_index(index);
            const size_type elem = _elem_index(index);
            chunks.resize_to_fit(chunkIndex + 1);

            auto chunk = chunks.write(chunkIndex);
            if (chunk->valid(elem)) {
                return { chunk->at(elem), false };
            }
            return { chunk->emplace(elem, std::forward<Args>(args)...), true };
        }

        /**
         * @brief Delete an object at a specific index
         *
         * @return bool True if an element was erased, false otherwise.
         */
        bool erase(size_type index)
        {
            return chunks.write(_chunk_index(index))->erase(_elem_index(index));
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

        /**
         * @brief Atomically create a copy of an element
         *
         * @param size_type index
         *
         * @return value_type A copy of the value at `index`. Requires
         *                    `value_type` to be copy-constructible.
         *
         * @throw std::out_of_range if no element exists at index `index`.
         */
        auto copyAtomically(size_type index) const -> value_type
            requires std::copy_constructible<value_type>
        {
            auto chunk = chunks.read(_chunk_index(index));
            return value_type{ chunk->at(_elem_index(index)) };
        }

        /**
         * @brief Perform an atomic operation on the element currently
         *        pointed to by the iterator
         *
         * @tparam F A type invocable with a `reference`.
         * @param size_type index
         * @param F&& func A function to be mapped to the element at `index` in
         *                 a way that is atomic with respect to other
         *                 operations on the SafeVector.
         *
         * @return auto The return value of `func`.
         *
         * @throw std::out_of_range if no element exists at index `index`.
         */
        template<std::invocable<reference> F>
        auto applyAtomically(size_type index, F&& func)
            -> std::invoke_result_t<F, reference>
        {
            auto chunk = chunks.write(_chunk_index(index));
            if constexpr (std::same_as<void, std::invoke_result_t<F, reference>>) {
                func(chunk->at(_elem_index(index)));
            }
            else {
                return func(chunk->at(_elem_index(index)));
            }
        }

        /**
         * @brief Perform an atomic operation on the element currently
         *        pointed to by the iterator
         *
         * @tparam F A type invocable with a `const_reference`.
         * @param size_type index
         * @param F&& func A function to be mapped to the element at `index` in
         *                 a way that is atomic with respect to other
         *                 operations on the SafeVector.
         *
         * @return auto The return value of `func`.
         *
         * @throw std::out_of_range if no element exists at index `index`.
         */
        template<std::invocable<const_reference> F>
        auto applyAtomically(size_type index, F&& func) const
            -> std::invoke_result_t<F, const_reference>
        {
            auto chunk = chunks.read(_chunk_index(index));
            if constexpr (std::same_as<void, std::invoke_result_t<F, reference>>) {
                func(chunk->at(_elem_index(index)));
            }
            else {
                return func(chunk->at(_elem_index(index)));
            }
        }

        auto begin()        -> iterator;
        auto begin()  const -> const_iterator;
        auto end()          -> iterator;
        auto end()    const -> const_iterator;

        auto cbegin() const -> const_iterator;
        auto cend()   const -> const_iterator;

    private:
        using Chunk = data::OptionalStorage<value_type, ChunkSize>;

        /**
         * We need to be able to lock single chunks to ensure atomicity of
         * operations like `emplace` or `erase`.
         */
        data::ElementLockableVector<Chunk> chunks;

    public:
        template<bool Constant>
        class Iterator
        {
        private:
            using ChunkPtr = std::conditional_t<Constant, const Chunk*, Chunk*>;
            using Self = std::conditional_t<Constant, const SafeVector, SafeVector>;

        public:
            using iterator_category = std::forward_iterator_tag;

            using value_type = SafeVector::value_type;
            using reference  = std::conditional_t<Constant, const value_type&, value_type&>;
            using pointer    = std::conditional_t<Constant, const value_type*, value_type*>;

            Iterator(Self& vec, size_type index);

            auto operator*() -> reference
            {
                assert(currentChunk != nullptr
                       && "Access to an invalid iterator (is this a past-the-end iterator?)");
                return currentChunk->at(_elem_index(currentIndex));
            }

            auto operator->() -> pointer
            {
                assert(currentChunk != nullptr
                       && "Access to an invalid iterator (is this a past-the-end iterator?)");
                return &currentChunk->at(_elem_index(currentIndex));
            }

            auto operator++() -> Iterator&;
            auto operator++(int) -> Iterator;

            bool operator==(const Iterator& other) const {
                return self == other.self && currentIndex == other.currentIndex;
            }

            /**
             * @brief Get the index of the element currently pointed to by the
             *        iterator
             *
             * # Example
             * ```cpp
             * auto it = vec.begin();
             * assert(&*it == &vec.at(it.index()));
             * ```
             *
             * @return size_type
             */
            inline
            auto index() const -> size_type {
                return currentIndex;
            }

            /**
             * @brief Atomically create a copy of an element
             *
             * @param size_type index
             *
             * @return value_type A copy of the value at `index`. Requires
             *                    `value_type` to be copy-constructible.
             *
             * @throw std::out_of_range if no element exists at index `index`.
             */
            auto copyAtomically() const -> value_type
                requires std::copy_constructible<value_type>;

            /**
             * @brief Perform an atomic operation on the element currently
             *        pointed to by the iterator
             *
             * # Example
             * ```cpp
             *
             * for (auto it = vec.begin(); it != vec.end(); ++it) {
             *     it.applyAtomically([](T& val) { process(val); });
             * }
             * ```
             *
             * @tparam F A type invocable with a `reference`.
             * @param F&& func A function to be mapped to the current element in
             *                 a way that is atomic with respect to other
             *                 operations on the SafeVector.
             *
             * @return auto The return value of `func`.
             */
            template<std::invocable<reference> F>
            auto applyAtomically(F&& func) -> std::invoke_result_t<F, reference>
            {
                return self->applyAtomically(this->index(), std::forward<F>(func));
            }

            /**
             * @brief Perform an atomic operation on the element currently
             *        pointed to by the iterator
             *
             * # Example
             * ```cpp
             *
             * for (auto it = vec.cbegin(); it != vec.cend(); ++it) {
             *     it.applyAtomically([](const T& val) { process(val); });
             * }
             * ```
             *
             * @tparam F A type invocable with a `const_reference`.
             * @param F&& func A function to be mapped to the current element in
             *                 a way that is atomic with respect to other
             *                 operations on the SafeVector.
             *
             * @return auto The return value of `func`.
             */
            template<std::invocable<const_reference> F>
            auto applyAtomically(F&& func) const -> std::invoke_result_t<F, const_reference>
            {
                return self->applyAtomically(this->index(), std::forward<F>(func));
            }

        private:
            /**
             * We could implement this in a simpler and cleaner way by just
             * storing the index and calling `contains`, `at`, etc. at every
             * access. This turns out to be significantly slower.
             *
             * This more sophisticated approach also has the nice property that
             * elements pointed to by the iterator can't be overridden during
             * access.
             */

            Self* self;
            ChunkPtr currentChunk;
            size_type currentIndex;

            auto getChunk(size_type chunkIndex) -> ChunkPtr
            {
                if constexpr (Constant) {
                    auto chunk = self->chunks.read(chunkIndex);
                    return &*chunk;
                }
                else {
                    auto chunk = self->chunks.write(chunkIndex);
                    return &*chunk;
                }
            }
        };
    };



    template<typename T, size_t ChunkSize>
    auto SafeVector<T, ChunkSize>::begin() -> iterator
    {
        return iterator{ *this, 0 };
    }

    template<typename T, size_t ChunkSize>
    auto SafeVector<T, ChunkSize>::begin() const -> const_iterator
    {
        return const_iterator{ *this, 0 };
    }

    template<typename T, size_t ChunkSize>
    auto SafeVector<T, ChunkSize>::end() -> iterator
    {
        return iterator{ *this, chunks.size() * ChunkSize };
    }

    template<typename T, size_t ChunkSize>
    auto SafeVector<T, ChunkSize>::end() const -> const_iterator
    {
        return const_iterator{ *this, chunks.size() * ChunkSize };
    }

    template<typename T, size_t ChunkSize>
    auto SafeVector<T, ChunkSize>::cbegin() const -> const_iterator
    {
        return std::as_const(*this).begin();
    }

    template<typename T, size_t ChunkSize>
    auto SafeVector<T, ChunkSize>::cend() const -> const_iterator
    {
        return std::as_const(*this).end();
    }



    template<typename T, size_t ChunkSize>
    template<bool Constant>
    SafeVector<T, ChunkSize>::Iterator<Constant>::Iterator(Self& vec, size_type index)
        :
        self(&vec),
        currentChunk(nullptr),
        currentIndex(index)
    {
        if (self->chunks.size() > _chunk_index(currentIndex))
        {
            currentChunk = getChunk(_chunk_index(currentIndex));

            const size_type elem = _elem_index(currentIndex);
            assert(currentChunk != nullptr);
            assert(currentChunk->size() > elem);
            if (!currentChunk->valid(elem)) {
                ++*this;
            }
        }
    }

    template<typename T, size_t ChunkSize>
    template<bool Constant>
    auto SafeVector<T, ChunkSize>::Iterator<Constant>::operator++() -> Iterator&
    {
        size_type currentChunkIndex = _chunk_index(currentIndex);

        while (currentChunk != nullptr)
        {
            ++currentIndex;

            const size_type newChunkIndex = _chunk_index(currentIndex);
            if (newChunkIndex >= self->chunks.size())
            {
                currentChunk = nullptr;
                break;
            }

            // Handle chunk transition
            if (currentChunkIndex < newChunkIndex)
            {
                assert(newChunkIndex == currentChunkIndex + 1);
                currentChunk = getChunk(newChunkIndex);
                ++currentChunkIndex;
            }

            assert(currentChunk != nullptr);
            assert(currentChunk->size() > _elem_index(currentIndex));

            // Break if the current element is valid
            if (currentChunk->valid(_elem_index(currentIndex))) {
                break;
            }
        }

        return *this;
    }

    template<typename T, size_t ChunkSize>
    template<bool Constant>
    auto SafeVector<T, ChunkSize>::Iterator<Constant>::operator++(int) -> Iterator
    {
        auto prev = *this;
        ++*this;
        return prev;
    }
} // namespace trc::util
