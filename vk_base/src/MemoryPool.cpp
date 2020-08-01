#include "MemoryPool.h"

#include <glm/glm.hpp>



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

auto vkb::ManagedMemoryChunk::allocateMemory(const vk::MemoryRequirements& requirements)
    -> DeviceMemory
{
    if (nextMemoryOffset + requirements.size > size) {
        throw std::out_of_range("Chunk is out of device memory");
    }

    const auto currentOffset = nextMemoryOffset;
    nextMemoryOffset += requirements.size;
    return DeviceMemory(
        { *memory, requirements.size, currentOffset },
        [this](const DeviceMemoryInternals& internals) {
            releaseMemory(internals.baseOffset, internals.size);
        }
    );
}

auto vkb::ManagedMemoryChunk::getRemainingSize() const noexcept -> vk::DeviceSize
{
    return size - nextMemoryOffset;
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

    if constexpr (enableVerboseLogging) {
        std::cout << "(ManagedMemoryChunk): Free " << memorySize << " bytes at offset "
            << memoryOffset << "\n";
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

vkb::MemoryPool::MemoryPool(const Device& device, vk::DeviceSize chunkSize)
    :
    device(&device),
    chunkSize(chunkSize),
    chunksPerMemoryType(device.getPhysicalDevice().memoryProperties.memoryTypeCount)
{
    if constexpr (vkb::enableVerboseLogging) {
        std::cout << "Memory pool created for " << chunksPerMemoryType.size() << " memory types.\n";
    }
}

void vkb::MemoryPool::setDevice(const Device& device)
{
    assert(this->device == nullptr);

    chunksPerMemoryType.resize(device.getPhysicalDevice().memoryProperties.memoryTypeCount);
    this->device = &device;
}

auto vkb::MemoryPool::allocateMemory(vk::MemoryPropertyFlags properties, vk::MemoryRequirements requirements)
    -> DeviceMemory
{
    if constexpr (enableVerboseLogging) {
        std::cout << "(MemoryPool): Request " << requirements.size << " bytes of memory\n";
    }

    uint32_t typeIndex = device->getPhysicalDevice().findMemoryType(
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

            if constexpr (enableVerboseLogging) {
                std::cout << "(MemoryPool): Allocate a new chunk of " << newChunkSize << " bytes"
                    << " and memory type " << typeIndex << "\n";
            }

            return chunks.emplace_back(
                new ManagedMemoryChunk(*device, newChunkSize, typeIndex)
            )->allocateMemory(requirements);
        }

        if (chunks[i]->getRemainingSize() >= requirements.size)
        {
            if constexpr (enableVerboseLogging) {
                std::cout << "(MemoryPool): Allocate " << requirements.size << " from chunk "
                    << i << " (" << chunks[i]->getRemainingSize() << " bytes remaining)\n";
            }
            return chunks.at(i)->allocateMemory(requirements);
        }

        i++;
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

void vkb::MemoryPool::reset()
{
    chunksPerMemoryType.erase(chunksPerMemoryType.begin(), chunksPerMemoryType.end());
}
