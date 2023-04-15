#include "trc/base/MemoryPool.h"

#include <stdexcept>

#include <glm/glm.hpp>

#include "trc/base/Logging.h"



trc::ManagedMemoryChunk::ManagedMemoryChunk(
    const Device& device,
    vk::DeviceSize size,
    uint32_t memoryTypeIndex)
    :
    memory(device->allocateMemoryUnique(vk::MemoryAllocateInfo{ size, memoryTypeIndex })),
    size(size)
{}

trc::ManagedMemoryChunk::ManagedMemoryChunk(
    const Device& device,
    vk::DeviceSize size,
    uint32_t memoryTypeIndex,
    vk::MemoryAllocateFlags allocateFlags)
    :
    memory([&] {
        vk::StructureChain chain{
            vk::MemoryAllocateInfo{ size, memoryTypeIndex },
            vk::MemoryAllocateFlagsInfo{ allocateFlags }
        };
        return device->allocateMemoryUnique(chain.get<vk::MemoryAllocateInfo>());
    }()),
    size(size)
{}

auto trc::ManagedMemoryChunk::allocateMemory(const vk::MemoryRequirements& requirements)
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

auto trc::ManagedMemoryChunk::getRemainingSize() const noexcept -> vk::DeviceSize
{
    return size - nextMemoryOffset;
}

void trc::ManagedMemoryChunk::releaseMemory(
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

    log::info << "(ManagedMemoryChunk): Free " << memorySize << " bytes at offset " << memoryOffset;
}



trc::MemoryPool::MemoryPool(
    const Device& device,
    vk::DeviceSize chunkSize,
    vk::MemoryAllocateFlags memoryAllocateFlags)
    :
    device(&device),
    chunkSize(chunkSize),
    memoryAllocateFlags(memoryAllocateFlags),
    chunksPerMemoryType(device.getPhysicalDevice().memoryProperties.memoryTypeCount)
{
    log::info << "Memory pool created for " << chunksPerMemoryType.size() << " memory types.";
}

auto trc::MemoryPool::allocateMemory(vk::MemoryPropertyFlags properties, vk::MemoryRequirements requirements)
    -> DeviceMemory
{
    log::info << "(MemoryPool): Request " << requirements.size << " bytes of memory";

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

            log::info << "(MemoryPool): Allocate a new chunk of " << newChunkSize << " bytes"
                      << " and memory type " << typeIndex;

            return chunks.emplace_back(
                memoryAllocateFlags
                    ?  new ManagedMemoryChunk(*device, newChunkSize, typeIndex, memoryAllocateFlags)
                    :  new ManagedMemoryChunk(*device, newChunkSize, typeIndex)
            )->allocateMemory(requirements);
        }

        if (chunks[i]->getRemainingSize() >= requirements.size)
        {
            log::info << "(MemoryPool): Allocate " << requirements.size << " from chunk "
                      << i << " (" << chunks[i]->getRemainingSize() << " bytes remaining)";
            return chunks.at(i)->allocateMemory(requirements);
        }

        i++;
    }
}

auto trc::MemoryPool::makeAllocator() -> DeviceMemoryAllocator
{
    return [this](const Device&,
                  vk::MemoryPropertyFlags properties,
                  vk::MemoryRequirements requirements)
    {
        return allocateMemory(properties, requirements);
    };
}

void trc::MemoryPool::reset()
{
    chunksPerMemoryType.erase(chunksPerMemoryType.begin(), chunksPerMemoryType.end());
}
