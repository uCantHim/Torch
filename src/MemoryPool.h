#ifndef MEMORYPOOL_H
#define MEMORYPOOL_H

#include <vector>
#include <optional>

#include <glm/glm.hpp>

#include "vkb/VulkanBase.h"

class ManagedMemoryChunk;


/**
 * @brief Self managed raw device memory
 *
 * Behaves like a shared_ptr to device memory.
 */
class DeviceMemory
{
    friend class ManagedMemoryChunk;

    DeviceMemory(
        ManagedMemoryChunk& chunk,
        vk::DeviceMemory mem,
        vk::DeviceSize size,
        vk::DeviceSize chunkOffset);

public:
    void bindToBuffer(const vk::Buffer& buffer);

    /**
     * @brief Map device memory to host memory
     *
     * Attention: Only one DeviceMemory from the same ManagedMemoryChunk
     * may be mapped at any time.
     *
     * Must not be called on memory that is already mapped.
     *
     * @param DeviceSize mappedSize Size of the mapped memory in bytes.
     * Must be less than or equal to the total memory size. May be
     * VK_WHOLE_SIZE.
     * @param DeviceSize offset     Offset in bytes from the start of the
     * memory.
     *
     * @return void* Pointer to mapped memory
     */
    auto map(vk::DeviceSize mappedSize, vk::DeviceSize offset) -> void*;

    /**
     * @brief Unmap the memory from host memory.
     *
     * Must not be called on memory that is not currently mapped.
     */
    void unmap();

private:
    std::shared_ptr<ManagedMemoryChunk> chunk;
    vk::DeviceMemory memory;
    vk::DeviceSize size; // Size of the memory
    vk::DeviceSize baseOffset; // Offset in the memory chunk
};


class ManagedMemoryChunk
{
public:
    /**
     * @brief A large chunk of device memory to allocate subranges from
     *
     * @param DeviceSize size            Size of memory in bytes
     * @param uint32_t   memoryTypeIndex The memory type is an index
     * returned from PhysicalDevice::findMemoryIndex().
     */
    ManagedMemoryChunk(vk::DeviceSize size, uint32_t memoryTypeIndex)
        :
        memory(vkb::VulkanBase::getDevice()->allocateMemoryUnique({ size, memoryTypeIndex })),
        size(size)
    {}

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
     * @return Allocated device memory if the allocation is successful.
     *         Nothing if all memory in the chunk has already been
     *         allocated.
     */
    auto allocateMemory(vk::MemoryRequirements requirements) noexcept -> std::optional<DeviceMemory>;

    /**
     * @brief Release allocated memory back to the pool
     *
     * Does not need to be called manually, allocated memory is self-
     * managing.
     */
    void releaseMemory(const DeviceMemory& freedMem) noexcept;

private:
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
    MemoryPool();

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
    [[nodiscard]]
    auto allocateMemory(vk::MemoryPropertyFlags properties, vk::MemoryRequirements requirements)
        -> DeviceMemory;

private:
    static constexpr vk::DeviceSize DEFAULT_CHUNK_SIZE = 100000000; // 200 mb, is divisible by 256 (base alignment)

    std::vector<std::vector<ManagedMemoryChunk>> chunksPerMemoryType;
};

#endif
