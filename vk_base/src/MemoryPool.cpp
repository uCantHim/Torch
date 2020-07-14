#include "MemoryPool.h"



vkb::ManagedMemoryChunk::ManagedMemoryChunk(vk::DeviceSize size, uint32_t memoryTypeIndex)
    :
    ManagedMemoryChunk(vkb::VulkanBase::getDevice(), size, memoryTypeIndex)
{}

vkb::ManagedMemoryChunk::ManagedMemoryChunk(
    const Device& device,
    vk::DeviceSize size,
    uint32_t memoryTypeIndex)
    :
    memory(device->allocateMemoryUnique({ size, memoryTypeIndex })),
    size(size)
{}

auto vkb::ManagedMemoryChunk::allocateMemory(vk::MemoryRequirements requirements)
    -> DeviceMemory
{
    if (nextMemoryOffset + requirements.size > size) {
        throw std::out_of_range("Chunk is out of device memory");
    }

    nextMemoryOffset += requirements.size;
    return DeviceMemory(
        { *memory, requirements.size, nextMemoryOffset },
        [this](const DeviceMemoryInternals& internals) {
            releaseMemory(internals.baseOffset, internals.size);
        }
    );
}

void vkb::ManagedMemoryChunk::releaseMemory(
    vk::DeviceSize memoryOffset,
    vk::DeviceSize memorySize) noexcept
{
    allocatedBuffers--;

    // Test if freed memory was the last allocated piece of memory in the chunk
    if (allocatedBuffers == 0)
    {
        nextMemoryOffset = 0;
    }
    // Test if freed memory was at the back of the chunk
    else if (nextMemoryOffset - memorySize == memoryOffset)
    {
        nextMemoryOffset -= memorySize;
    }
}



vkb::MemoryPool::MemoryPool(vk::DeviceSize chunkSize)
    :
    chunkSize(chunkSize)
{
    if constexpr (vkb::enableVerboseLogging) {
        std::cout << "Memory pool created with deferred physical device initialization\n";
    }
}

vkb::MemoryPool::MemoryPool(const PhysicalDevice& physDevice, vk::DeviceSize chunkSize)
    :
    physDevice(&physDevice),
    chunkSize(chunkSize),
    chunksPerMemoryType(physDevice.memoryProperties.memoryTypeCount)
{
    if constexpr (vkb::enableVerboseLogging) {
        std::cout << "Memory pool created for " << chunksPerMemoryType.size() << " memory types.\n";
    }
}

void vkb::MemoryPool::setPhysicalDevice(const PhysicalDevice& physDevice)
{
    chunksPerMemoryType.resize(physDevice.memoryProperties.memoryTypeCount);
    this->physDevice = &physDevice;
}

auto vkb::MemoryPool::allocateMemory(vk::MemoryPropertyFlags properties, vk::MemoryRequirements requirements)
    -> DeviceMemory
{
    uint32_t typeIndex = physDevice->findMemoryType(
        requirements.memoryTypeBits,
        properties
    );

    auto& chunks = chunksPerMemoryType[typeIndex];
    size_t i = 0;
    while (true)
    {
        if (chunks.size() <= i)
        {
            // No more memory chunks to allocate from.
            // Create a new memory chunk for the required memory type index.
            const size_t newChunkSize = glm::max(chunkSize, requirements.size);

            if constexpr (vkb::enableVerboseLogging) {
                std::cout << "Allocated a chunk of " << newChunkSize << " bytes of device memory.\n";
            }

            return chunks.emplace_back(
                newChunkSize,
                typeIndex
            ).allocateMemory(requirements);
        }

        try
        {
            return chunks.at(i).allocateMemory(requirements);
        }
        catch (const std::out_of_range& err)
        {
            // This out-of-range is thrown by ManagedMemoryChunk
            i++;
            continue;
        }
    }
}

auto vkb::MemoryPool::makeAllocator() -> DeviceMemoryAllocator
{
    return [this](const Device&,
                  vk::MemoryPropertyFlags properties,
                  vk::MemoryRequirements requirements)
    {
        return allocateMemory(properties, requirements);
    };
}
