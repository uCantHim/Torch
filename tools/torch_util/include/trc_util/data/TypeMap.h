#pragma once

#include <cstdint>

#include <atomic>
#include <vector>

namespace trc::data
{
    /**
     * @brief Allocate static indices for types
     *
     * @tparam Tag A type tag to create different allocators that each start
     *             at the index 0.
     */
    template<typename Tag>
    struct TypeIndexAllocator
    {
    public:
        TypeIndexAllocator() = default;

        template<typename U>
        static auto get() -> uint32_t
        {
            static uint32_t index{ allocTypeIndex() };
            return index;
        }

    private:
        static auto allocTypeIndex() -> uint32_t
        {
            static std::atomic<uint32_t> next{ 0 };
            return next++;
        }
    };
} // namespace trc::data
