#pragma once

#include <cassert>
#include <cstdint>
#include <array>
#include <bitset>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <vector>

namespace trc::util
{
    using std::size_t;

    namespace impl
    {
        template<typename T, size_t ChunkSize>
        class SafeVectorBase
        {
        protected:
            using value_type = T;
            using pointer = T*;
            using const_pointer = const T*;
            using reference = T&;
            using const_reference = const T&;
            using size_type = size_t;

            static constexpr size_type chunk_size{ ChunkSize };

            static constexpr inline auto _chunk_index(size_type index) -> size_type {
                return index / chunk_size;
            }
            static constexpr inline auto _elem_index(size_type index) -> size_type {
                return index % chunk_size;
            }
        };

        template<typename T, size_t ChunkSize>
        class SafeVectorGuardedImpl : public SafeVectorBase<T, ChunkSize>
        {
        private:
            using Base = SafeVectorBase<T, ChunkSize>;

            using Base::_chunk_index;
            using Base::_elem_index;

        protected:
            using typename Base::value_type;
            using typename Base::reference;
            using typename Base::const_reference;
            using typename Base::pointer;
            using typename Base::const_pointer;
            using typename Base::size_type;

            inline bool _is_valid_elem(const size_type index) const
            {
                const size_type chunk = _chunk_index(index);

                std::shared_lock lock(chunkListAccessLock);
                return chunks.size() > chunk && get_chunk(chunk).valid(_elem_index(index));
            }

            auto _access_elem(size_type index) -> reference
            {
                check_valid_elem(index);
                return get_chunk(_chunk_index(index)).at(_elem_index(index));
            }

            auto _access_elem(size_type index) const -> const_reference
            {
                check_valid_elem(index);
                return get_chunk(_chunk_index(index)).at(_elem_index(index));
            }

            template<typename ...Args>
            void _construct_elem(size_type index, Args&&... args)
            {
                if (_is_valid_elem(index)) {
                    throw std::runtime_error("");
                }

                get_chunk(_chunk_index(index)).emplace(
                    _elem_index(index),
                    std::forward<Args>(args)...
                );
            }

            void _delete_elem(size_type index)
            {
                check_valid_elem(index);
                get_chunk(_chunk_index(index)).erase(_elem_index(index));
            }

            void _resize_to_fit(size_type index)
            {
                const size_type chunk = _chunk_index(index);
                if (chunks.size() <= chunk)
                {
                    std::scoped_lock lock(chunkListAccessLock);
                    while (chunks.size() <= chunk) {
                        chunks.emplace_back(std::make_unique<Chunk>());
                    }
                }
            }

            void _clear()
            {
                std::scoped_lock lock(chunkListAccessLock);
                chunks.clear();
            }

        private:
            struct Chunk
            {
                Chunk(const Chunk&) = delete;
                Chunk(Chunk&&) = delete;
                Chunk& operator=(const Chunk&) = delete;
                Chunk& operator=(Chunk&&) = delete;

                Chunk() = default;
                ~Chunk()
                {
                    // Delete all valid elements
                    for (size_type i = 0; i < Base::chunk_size; ++i)
                    {
                        if (valid(i)) {
                            erase(i);
                        }
                    }
                }

                inline bool valid(size_type index) const
                {
                    assert(index < Base::chunk_size);
                    return validBits[index];
                }

                inline auto at(size_type index) -> reference
                {
                    assert(valid(index) && "Never call at for an invalid element!");
                    return reinterpret_cast<pointer>(data.data())[index];
                }

                inline auto at(size_type index) const -> const_reference
                {
                    assert(valid(index) && "Never call at for an invalid element!");
                    return reinterpret_cast<const_pointer>(data.data())[index];
                }

                template<typename ...Args>
                inline void emplace(size_type index, Args&&... args)
                {
                    assert(!valid(index) && "Never call emplace for an already existing element!");
                    std::byte* dataPtr = data.data() + index * sizeof(value_type);
                    new (dataPtr) value_type(std::forward<Args>(args)...);
                    validBits[index] = true;
                }

                inline void erase(size_type index)
                {
                    assert(valid(index) && "Never call erase for an invalid element!");
                    at(index).~value_type();
                    validBits[index] = false;
                }

            private:
                std::bitset<Base::chunk_size> validBits{ 0 };
                std::array<std::byte, Base::chunk_size * sizeof(value_type)> data{ std::byte{0} };
            };

            auto get_chunk(size_type chunk_index) -> Chunk&
            {
                std::shared_lock lock(chunkListAccessLock);
                assert(chunks.size() > chunk_index);
                assert(chunks.at(chunk_index) != nullptr);
                return *chunks.at(chunk_index);
            }

            auto get_chunk(size_type chunk_index) const -> Chunk&
            {
                std::shared_lock lock(chunkListAccessLock);
                assert(chunks.size() > chunk_index);
                assert(chunks.at(chunk_index) != nullptr);
                return *chunks.at(chunk_index);
            }

            inline void check_valid_elem(size_type index) const
            {
                if (!_is_valid_elem(index)) {
                    throw std::out_of_range("No element exists at index "
                                            + std::to_string(index) + "!");
                }
            }

            mutable std::shared_mutex chunkListAccessLock;
            std::vector<std::unique_ptr<Chunk>> chunks;
        };
    } // namespace impl

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
     * 'Thread-safe' because all operations on the vector are guarded so
     * that multiple threads can call read AND write operations
     * concurrently. Access to the elements themselves is of course NOT
     * secure.
     *
     * Memory is organized in chunks. Once a chunk is allocated, it will
     * live forever. The application should use the vector in a way that
     * its space is populated as densely as possible.
     *
     * Chunk size can be controlled via the template parameter `ChunkSize`.
     * A chunk will occupy `ChunkSize * sizeof(T)` bytes in memory.
     *
     * TODO: Implementing an iterator for SafeVector is very possible.
     */
    template<
        typename T,
        size_t ChunkSize = kSafeVectorDefaultChunkSize
        >
    class SafeVector : protected impl::SafeVectorGuardedImpl<T, ChunkSize>
    {
    public:
        using Impl = impl::SafeVectorGuardedImpl<T, ChunkSize>;
        using Base = Impl;

        using typename Base::value_type;
        using typename Base::reference;
        using typename Base::const_reference;
        using typename Base::pointer;
        using typename Base::const_pointer;
        using typename Base::size_type;

        /**
         * @brief Test whether an element exists at a specific index
         *
         * @return bool
         */
        bool contains(size_type index) const {
            return Impl::_is_valid_elem(index);
        }

        /**
         * @brief Retrieve an element at a specific index
         *
         * @throw std::out_of_range if no element exists at index `index`.
         */
        auto at(size_type index) -> reference {
            return Impl::_access_elem(index);
        }

        /**
         * @brief Retrieve an element at a specific index
         *
         * @throw std::out_of_range if no element exists at index `index`.
         */
        auto at(size_type index) const -> const_reference {
            return Impl::_access_elem(index);
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
            if (Impl::_is_valid_elem(index)) {
                Impl::_delete_elem(index);
            }
            else {
                Impl::_resize_to_fit(index);
            }

            Impl::_construct_elem(index, std::forward<Args>(args)...);
            assert(Impl::_is_valid_elem(index));
            return Impl::_access_elem(index);
        }

        /**
         * @brief Delete an object at a specific index
         *
         * @throw std::out_of_range if no element exists at index `index`.
         */
        void erase(size_type index)
        {
            Impl::_delete_elem(index);
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
            Impl::_resize_to_fit(new_size);
        }

        /**
         * @brief Destroy all elements and deallocate all storage space
         */
        void clear()
        {
            Impl::_clear();
        }
    };
} // namespace trc::util
