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
    other.isMovedFrom = true;
}

vkb::DeviceMemory::~DeviceMemory()
{
    if (!isMovedFrom) {
        deleter(internal);
        std::cout << "Device memory destroyed\n";
    }
}

auto vkb::DeviceMemory::operator=(DeviceMemory&& rhs) noexcept -> DeviceMemory&
{
    std::swap(deleter, rhs.deleter);
    std::swap(internal, rhs.internal);

    isMovedFrom = false;
    rhs.isMovedFrom = true;

    return *this;
}

void vkb::DeviceMemory::bindToBuffer(const Device& device, vk::Buffer buffer)
{
    device->bindBufferMemory(buffer, internal.memory, internal.baseOffset);
}

auto vkb::DeviceMemory::map(const Device& device, vk::DeviceSize mappedSize, vk::DeviceSize offset) -> void*
{
    assert(mappedSize == VK_WHOLE_SIZE || mappedSize <= internal.size);

    if (mappedSize == VK_WHOLE_SIZE) {
        mappedSize = internal.size;
    }
    return device->mapMemory(internal.memory, internal.baseOffset + offset, mappedSize);
}

void vkb::DeviceMemory::unmap(const Device& device)
{
    device->unmapMemory(internal.memory);
}

auto vkb::DeviceMemory::getSize() const noexcept -> vk::DeviceSize
{
    return internal.size;
}
