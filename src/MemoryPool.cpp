#include "MemoryPool.h"



DeviceMemory::DeviceMemory(
    ManagedMemoryChunk& chunk,
    vk::DeviceMemory mem,
    vk::DeviceSize size,
    vk::DeviceSize chunkOffset)
    :
    chunk(std::shared_ptr<ManagedMemoryChunk>(
        &chunk,
        [&](ManagedMemoryChunk* chunk) {
            chunk->releaseMemory(*this);
        }
    )),
    memory(mem),
    size(size),
    baseOffset(chunkOffset)
{
    if constexpr (vkb::enableVerboseLogging)
        std::cout << size << " bytes of pooled memory have been allocated.\n";
}

void DeviceMemory::bindToBuffer(const vk::Buffer& buffer)
{
    assert(vkb::VulkanBase::getDevice()->getBufferMemoryRequirements(buffer).size == size);

    vkb::VulkanBase::getDevice()->bindBufferMemory(buffer, memory, baseOffset);
}

auto DeviceMemory::map(vk::DeviceSize mappedSize, vk::DeviceSize offset) -> void*
{
    assert(mappedSize == VK_WHOLE_SIZE || mappedSize <= size);

    if (mappedSize == VK_WHOLE_SIZE)
        mappedSize = size;
    return vkb::VulkanBase::getDevice()->mapMemory(memory, baseOffset + offset, mappedSize);
}

void DeviceMemory::unmap()
{
    vkb::VulkanBase::getDevice()->unmapMemory(memory);
}



auto ManagedMemoryChunk::allocateMemory(vk::MemoryRequirements requirements) noexcept
    -> std::optional<DeviceMemory>
{
    if (nextMemoryOffset + requirements.size > size)
        return std::nullopt;

    auto result = DeviceMemory(*this, *memory, requirements.size, nextMemoryOffset);
    nextMemoryOffset += requirements.size;
    return result;
}

void ManagedMemoryChunk::releaseMemory(const DeviceMemory& freedMem) noexcept
{
    allocatedBuffers--;
    if (allocatedBuffers == 0)
        nextMemoryOffset = 0;
    // Test if freed memory was the last allocated piece of memory in the chunk
    else if (nextMemoryOffset - freedMem.size == freedMem.baseOffset)
        nextMemoryOffset -= freedMem.size;
}



MemoryPool::MemoryPool()
{
    chunksPerMemoryType.resize(
        vkb::VulkanBase::getDevice().getPhysicalDevice().memoryProperties.memoryTypeCount
    );
    if constexpr (vkb::enableVerboseLogging)
        std::cout << "Memory pool created for " << chunksPerMemoryType.size() << " memory types.\n";
}

auto MemoryPool::allocateMemory(vk::MemoryPropertyFlags properties, vk::MemoryRequirements requirements)
    -> DeviceMemory
{

    uint32_t typeIndex = vkb::VulkanBase::getDevice().getPhysicalDevice().findMemoryType(
        requirements.memoryTypeBits,
        properties
    );

    size_t i = 0;
    while (true)
    {
        try {
            auto mem = chunksPerMemoryType[typeIndex].at(i).allocateMemory(requirements);
            if (!mem.has_value())
            {
                // The tested memory chunk has no memory available, try the next one
                i++;
                continue;
            }
            return mem.value();
        }
        catch (const std::out_of_range& err) {
            // No more memory chunks to allocate from
            // Create a new memory chunk for the required memory type index
            if constexpr (vkb::enableVerboseLogging)
            {
                size_t chunkSize = glm::max(DEFAULT_CHUNK_SIZE, requirements.size);
                auto result = chunksPerMemoryType[typeIndex].emplace_back(
                    chunkSize,
                    typeIndex
                ).allocateMemory(requirements).value();
                std::cout << "Allocated a chunk of " << chunkSize << " bytes of device memory.\n";
            }
            else
            {
                return chunksPerMemoryType[typeIndex].emplace_back(
                    glm::max(DEFAULT_CHUNK_SIZE, requirements.size),
                    typeIndex
                ).allocateMemory(requirements).value();
            }
        }
    }
}
