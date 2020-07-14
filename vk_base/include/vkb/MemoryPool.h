#pragma once

#include <vector>

#include <glm/glm.hpp>

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
         * @param DeviceSize size            Size of memory in bytes
         * @param uint32_t   memoryTypeIndex The memory type is an index returned from
         *                                   PhysicalDevice::findMemoryIndex().
         */
        ManagedMemoryChunk(const Device& device, vk::DeviceSize size, uint32_t memoryTypeIndex);

        /**
         * @brief Allocate raw device memory from the pre-allocated chunk
         *
         * The memory requirements structure is not really needed here since
         * the chunk already has a memory type defined. It is only used to
         * tell the chunk what size the allocation should be. It also brigns
         * some consistency.
         *
         * @param MemoryRequirements requirements
         *
         * @return Allocated device memory.
         *
         * @throw std::out_of_range if not enough memory is available in
         *                          the chunk.
         */
        auto allocateMemory(vk::MemoryRequirements requirements) -> DeviceMemory;

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
     * Does not use any device memory when created. Memory is allocated
     * on a on-use-basis.
     *
     * One may create an arbitrary amount of memory pools.
     *
     * The pool just manages multiple ManagedMemoryChunks which allocate
     * device memory themselves.
     */
    class MemoryPool
    {
    public:
        explicit MemoryPool(vk::DeviceSize chunkSize = DEFAULT_CHUNK_SIZE);
        explicit MemoryPool(const PhysicalDevice& physDevice, vk::DeviceSize chunkSize = DEFAULT_CHUNK_SIZE);

        void setPhysicalDevice(const PhysicalDevice& physDevice);

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

        auto makeAllocator() -> DeviceMemoryAllocator;

    private:
        static constexpr vk::DeviceSize DEFAULT_CHUNK_SIZE = 100000000; // 100 mb, is divisible by 256 (base alignment)

        const PhysicalDevice* physDevice{ nullptr };
        vk::DeviceSize chunkSize{ DEFAULT_CHUNK_SIZE };
        std::vector<std::vector<ManagedMemoryChunk>> chunksPerMemoryType;
    };
} // namespace vkb
