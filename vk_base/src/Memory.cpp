#include "Memory.h"



vkb::DeviceMemory::DeviceMemory(DeviceMemoryInternals data, DeviceMemoryDeleter deleter)
    :
    deleter(std::move(deleter)),
    internal(data)
{
}

vkb::DeviceMemory::DeviceMemory(DeviceMemory&& other) noexcept
    :
    deleter(std::move(other.deleter)),
    internal(std::move(other.internal))
{
    other.deleter = [](...) {};
}

vkb::DeviceMemory::~DeviceMemory()
{
    deleter(internal);
}

auto vkb::DeviceMemory::operator=(DeviceMemory&& rhs) noexcept -> DeviceMemory&
{
    deleter = std::move(rhs.deleter);
    internal = std::move(rhs.internal);

    rhs.deleter = [](...) {};

    return *this;
}

auto vkb::DeviceMemory::allocate(
    const Device& device,
    vk::MemoryPropertyFlags properties,
    vk::MemoryRequirements requirements) -> DeviceMemory
{
    vk::DeviceMemory mem = device->allocateMemory({
        requirements.size,
        device.getPhysicalDevice().findMemoryType(
            requirements.memoryTypeBits, properties
        )
    });

    return DeviceMemory(
        { mem, requirements.size, 0 },
        [&device](const DeviceMemoryInternals& internals) {
            device->freeMemory(internals.memory);
        }
    );
}

void vkb::DeviceMemory::bindToBuffer(const Device& device, vk::Buffer buffer)
{
    device->bindBufferMemory(buffer, internal.memory, internal.baseOffset);
}

auto vkb::DeviceMemory::map(const Device& device,
                            vk::DeviceSize mappedOffset,
                            vk::DeviceSize mappedSize) -> void*
{
    assert(mappedSize == VK_WHOLE_SIZE || mappedSize <= internal.size);

    if (mappedSize == VK_WHOLE_SIZE) {
        mappedSize = internal.size;
    }
    return device->mapMemory(internal.memory, internal.baseOffset + mappedOffset, mappedSize);
}

void vkb::DeviceMemory::unmap(const Device& device)
{
    device->unmapMemory(internal.memory);
}

auto vkb::DeviceMemory::getSize() const noexcept -> vk::DeviceSize
{
    return internal.size;
}
