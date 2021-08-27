#include "Memory.h"



vkb::DeviceMemory::DeviceMemory(DeviceMemoryInternals data, DeviceMemoryDeleter deleter)
    :
    deleter(std::move(deleter)),
    internal(data)
{
}

vkb::DeviceMemory::DeviceMemory(DeviceMemory&& other) noexcept
{
    deleter(internal);

    deleter = std::move(other.deleter);
    internal = other.internal;

    other.deleter = [](const DeviceMemoryInternals&) {};
}

vkb::DeviceMemory::~DeviceMemory()
{
    deleter(internal);
}

auto vkb::DeviceMemory::operator=(DeviceMemory&& rhs) noexcept -> DeviceMemory&
{
    deleter(internal);

    deleter = std::move(rhs.deleter);
    internal = rhs.internal;

    rhs.deleter = [](const DeviceMemoryInternals&) {};

    return *this;
}

auto vkb::DeviceMemory::allocate(
    const Device& device,
    vk::MemoryPropertyFlags properties,
    vk::MemoryRequirements requirements) -> DeviceMemory
{
    // Device address feature is always available in Vulkan 1.2
    vk::StructureChain chain{
        vk::MemoryAllocateInfo{
            requirements.size,
            device.getPhysicalDevice().findMemoryType(requirements.memoryTypeBits, properties)
        },
        vk::MemoryAllocateFlagsInfo{
            vk::MemoryAllocateFlagBits::eDeviceAddress
        }
    };
    vk::DeviceMemory mem = device->allocateMemory(chain.get<vk::MemoryAllocateInfo>());

    return DeviceMemory(
        { mem, requirements.size, 0 },
        [&device](const DeviceMemoryInternals& internals) {
            device->freeMemory(internals.memory, {});
        }
    );
}

void vkb::DeviceMemory::bindToBuffer(const Device& device, vk::Buffer buffer) const
{
    device->bindBufferMemory(buffer, internal.memory, internal.baseOffset);
}

void vkb::DeviceMemory::bindToImage(const Device& device, vk::Image image) const
{
    device->bindImageMemory(image, internal.memory, internal.baseOffset);
}

auto vkb::DeviceMemory::map(const Device& device,
                            vk::DeviceSize mappedOffset,
                            vk::DeviceSize mappedSize) const -> void*
{
    assert(mappedSize == VK_WHOLE_SIZE || mappedSize <= internal.size);

    if (mappedSize == VK_WHOLE_SIZE) {
        mappedSize = internal.size - mappedOffset;
    }
    return device->mapMemory(internal.memory, internal.baseOffset + mappedOffset, mappedSize);
}

void vkb::DeviceMemory::unmap(const Device& device) const
{
    device->unmapMemory(internal.memory);
}

auto vkb::DeviceMemory::getSize() const noexcept -> vk::DeviceSize
{
    return internal.size;
}
