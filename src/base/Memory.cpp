#include "trc/base/Memory.h"



trc::DeviceMemory::DeviceMemory(DeviceMemoryInternals data, DeviceMemoryDeleter deleter)
    :
    deleter(std::move(deleter)),
    internal(data)
{
}

trc::DeviceMemory::DeviceMemory(DeviceMemory&& other) noexcept
{
    deleter(internal);

    deleter = std::move(other.deleter);
    internal = other.internal;

    other.deleter = [](const DeviceMemoryInternals&) {};
}

trc::DeviceMemory::~DeviceMemory()
{
    deleter(internal);
}

auto trc::DeviceMemory::operator=(DeviceMemory&& rhs) noexcept -> DeviceMemory&
{
    deleter(internal);

    deleter = std::move(rhs.deleter);
    internal = rhs.internal;

    rhs.deleter = [](const DeviceMemoryInternals&) {};

    return *this;
}

auto trc::DeviceMemory::allocate(
    const Device& device,
    vk::MemoryPropertyFlags properties,
    vk::MemoryRequirements requirements) -> DeviceMemory
{
    vk::DeviceMemory mem = device->allocateMemory({
        requirements.size,
        device.getPhysicalDevice().findMemoryType(requirements.memoryTypeBits, properties)
    });

    return DeviceMemory(
        { mem, requirements.size, 0 },
        [&device](const DeviceMemoryInternals& internals) {
            device->freeMemory(internals.memory, {});
        }
    );
}

auto trc::DeviceMemory::allocate(
    const Device& device,
    vk::MemoryPropertyFlags properties,
    vk::MemoryRequirements requirements,
    vk::MemoryAllocateFlags flags) -> DeviceMemory
{
    // Device address feature is always available in Vulkan 1.2
    vk::StructureChain chain{
        vk::MemoryAllocateInfo{
            requirements.size,
            device.getPhysicalDevice().findMemoryType(requirements.memoryTypeBits, properties)
        },
        vk::MemoryAllocateFlagsInfo{
            flags
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

void trc::DeviceMemory::bindToBuffer(const Device& device, vk::Buffer buffer) const
{
    auto pad = [](auto val, auto pad){ auto r=val%pad; return r == 0 ? val : val + (pad - r); };
    const vk::DeviceSize align = device->getBufferMemoryRequirements(buffer).alignment;
    const vk::DeviceSize paddedOffset = pad(internal.baseOffset, align);

    device->bindBufferMemory(buffer, internal.memory, paddedOffset);
}

void trc::DeviceMemory::bindToImage(const Device& device, vk::Image image) const
{
    auto pad = [](auto val, auto pad){ auto r=val%pad; return r == 0 ? val : val + (pad - r); };
    const vk::DeviceSize align = device->getImageMemoryRequirements(image).alignment;
    const vk::DeviceSize paddedOffset = pad(internal.baseOffset, align);

    device->bindImageMemory(image, internal.memory, paddedOffset);
}

auto trc::DeviceMemory::map(const Device& device,
                            vk::DeviceSize mappedOffset,
                            vk::DeviceSize mappedSize) const -> void*
{
    assert(mappedSize == VK_WHOLE_SIZE || mappedSize <= internal.size);

    if (mappedSize == VK_WHOLE_SIZE) {
        mappedSize = internal.size - mappedOffset;
    }
    return device->mapMemory(internal.memory, internal.baseOffset + mappedOffset, mappedSize);
}

void trc::DeviceMemory::unmap(const Device& device) const
{
    device->unmapMemory(internal.memory);
}

void trc::DeviceMemory::flush(const Device& device,
                              vk::DeviceSize offset,
                              vk::DeviceSize size) const
{
    device->flushMappedMemoryRanges(vk::MappedMemoryRange(internal.memory, offset, size));
}

auto trc::DeviceMemory::getSize() const noexcept -> vk::DeviceSize
{
    return internal.size;
}
