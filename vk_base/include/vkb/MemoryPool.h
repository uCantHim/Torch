#pragma once

#include <vector>

#include "VulkanBase.h"
#include "Memory.h"

namespace vkb
{
    /**
     * Is usually managed by MemoryPool, but can be created on its own.
     */
    class ManagedMemoryChunk
    {
    public:
        /**
         * @brief A large chunk of device memory to allocate subranges from
         *
         * @param DeviceSize size            Size of memory in bytes
         * @param uint32_t   memoryTypeIndex The memory type is an index returned from
         *                                   PhysicalDevice::findMemoryIndex().
         */
        ManagedMemoryChunk(vk::DeviceSize size, uint32_t memoryTypeIndex);

        /**
         * @brief A large chunk of device memory to allocate subranges from
         *
         * @param Device     device          The device to allocate memory from
         * @param DeviceSize size            Size of memory in bytes
         * @param uint32_t   memoryTypeIndex The memory type is an index returned from
         *                                   PhysicalDevice::findMemoryIndex().
         */
        ManagedMemoryChunk(const Device& device, vk::DeviceSize size, uint32_t memoryTypeIndex);

        ManagedMemoryChunk(const ManagedMemoryChunk&) = delete;
        ManagedMemoryChunk(ManagedMemoryChunk&&) = delete;
        auto operator=(const ManagedMemoryChunk&) -> ManagedMemoryChunk& = delete;
        auto operator=(ManagedMemoryChunk&&) -> ManagedMemoryChunk& = delete;
        ~ManagedMemoryChunk() = default;

        /**
         * @brief Allocate raw device memory from the pre-allocated chunk
         *
         * The memory requirements structure is not really needed here since
         * the chunk already has a memory type defined. It is only used to
         * tell the chunk what size the allocation should be. It improves
         * consistency to do it this way.
         *
         * @param MemoryRequirements requirements
         *
         * @return Allocated device memory.
         *
         * @throw std::out_of_range if not enough memory is available in
         *                          the chunk.
         */
        auto allocateMemory(const vk::MemoryRequirements& requirements) -> DeviceMemory;

        /**
         * @return vk::DeviceSize The amount of free bytes in the chunk
         */
        auto getRemainingSize() const noexcept -> vk::DeviceSize;

    private:
        /**
         * @brief Release allocated memory back to the pool
         *
         * Does not need to be called manually, allocated memory is self-
         * managing.
         */
        void releaseMemory(vk::DeviceSize memoryOffset, vk::DeviceSize memorySize) noexcept;

        vk::UniqueDeviceMemory memory;
        vk::DeviceSize size;
        vk::DeviceSize nextMemoryOffset{ 0 };
        vk::DeviceSize allocatedBuffers{ 0 };
    };


    /**
     * @brief A pool to manage device memory
     *
     * The use of a pool reduces the required number of individual device
     * memory allocations significantly. This is necessary because Vulkan's
     * current implementation limit for single memory allocations is 4096,
     * which could be reached quickly with trivial implementations.
     *
     * Does not allocate any device memory when created. Memory is
     * allocated on a on-use-basis.
     *
     * The pool manages multiple chunks of device memory from which device
     * memory is allocated.
     */
    class MemoryPool
    {
    public:
        /**
         * @brief Partial initialization
         *
         * One MUST call MemoryPool::setDevice() before trying to allocate
         * any memory from the pool.
         *
         * This constructor is meant for the purpose of initializing pools
         * at static object creation time. Always use other constructors if
         * you already have a physical available.
         *
         * @param vk::DeviceSize chunkSize Size of memory chunks allocated
         *                                 in the memory pool.
         */
        explicit MemoryPool(vk::DeviceSize chunkSize);

        /**
         * @brief Create a memory pool
         *
         * @param const Device&  device
         * @param vk::DeviceSize chunkSize Size of memory chunks allocated
         *                                 in the memory pool.
         */
        explicit MemoryPool(const Device& device,
                            vk::DeviceSize chunkSize = DEFAULT_CHUNK_SIZE);

        MemoryPool(const MemoryPool&) = delete;
        MemoryPool(MemoryPool&&) noexcept = default;
        auto operator=(const MemoryPool&) -> MemoryPool& = delete;
        auto operator=(MemoryPool&&) noexcept -> MemoryPool& = default;
        ~MemoryPool() = default;

        /**
         * @brief Set the used device for the memory pool
         *
         * Use this after creating the pool without an initial device,
         * but before any memory is allocated from the pool.
         *
         * Cannot be used if a device has already been assigned to the pool
         * (fails an assertion).
         */
        void setDevice(const Device& physDevice);

        /**
         * @brief Allocate device memory
         *
         * @param vk::MemoryPropertyFlags properties Desired properties of
         * the requested memory
         * @param vk::MemoryRequirements requirements Memory requirements
         * imposed by buffer creation
         *
         * @return DeviceMemory A self-managed piece of raw device memory
         *                      with the specified properties
         */
        auto allocateMemory(vk::MemoryPropertyFlags properties, vk::MemoryRequirements requirements)
            -> DeviceMemory;

        /**
         * @brief Create an allocator that allocates memory from the pool
         *
         * @return DeviceMemoryAllocator
         */
        auto makeAllocator() -> DeviceMemoryAllocator;

        /**
         * @brief Free all memory allocated in the pool
         */
        void reset();

    private:
        static constexpr vk::DeviceSize DEFAULT_CHUNK_SIZE = 100000000; // 100 mb, is divisible by 256 (base alignment)

        const Device* device{ nullptr };
        vk::DeviceSize chunkSize{ DEFAULT_CHUNK_SIZE };
        std::vector<std::vector<std::unique_ptr<ManagedMemoryChunk>>> chunksPerMemoryType;
    };
} // namespace vkb
