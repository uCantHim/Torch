#include "Buffer.h"



// ------------------------- //
//        Base buffer        //
// ------------------------- //

vkb::Buffer::Buffer(
    vk::DeviceSize bufferSize,
    vk::BufferUsageFlags usage,
    vk::MemoryPropertyFlags flags,
    MemoryAllocator allocator)
    :
    Buffer(vkb::VulkanBase::getDevice(), bufferSize, usage, flags, std::move(allocator))
{}


vkb::Buffer::Buffer(
    const Device& device,
    vk::DeviceSize bufferSize,
    vk::BufferUsageFlags usage,
    vk::MemoryPropertyFlags flags,
    MemoryAllocator allocator)
    :
    device(&device),
    buffer(device->createBufferUnique(
        vk::BufferCreateInfo{
            vk::BufferCreateFlags(),
            bufferSize,
            usage,
            vk::SharingMode::eExclusive,
            0,
            nullptr
        }
    )),
    memory(allocator(device, flags, device->getBufferMemoryRequirements(*buffer))),
    bufferSize(bufferSize)
{
    memory.bindToBuffer(device, *buffer);
}


vkb::Buffer::Buffer(
    vk::DeviceSize bufferSize,
    const void* data,
    vk::BufferUsageFlags usage,
    vk::MemoryPropertyFlags flags,
    MemoryAllocator allocator)
    :
    Buffer(vkb::VulkanBase::getDevice(), bufferSize, data, usage, flags, std::move(allocator))
{
}


vkb::Buffer::Buffer(
    const Device& device,
    vk::DeviceSize bufferSize,
    const void* data,
    vk::BufferUsageFlags usage,
    vk::MemoryPropertyFlags flags,
    MemoryAllocator allocator)
    :
    Buffer(device, bufferSize, usage, flags, std::move(allocator))
{
    copyFrom(bufferSize, data);
}


auto vkb::Buffer::map(vk::DeviceSize offset, vk::DeviceSize size) -> memptr
{
    return static_cast<memptr>(memory.map(*device, offset, size));
}


auto vkb::Buffer::map(BufferRegion mappedRegion) -> memptr
{
    return map(mappedRegion.offset, mappedRegion.size);
}


void vkb::Buffer::unmap()
{
    memory.unmap(*device);
}


void vkb::Buffer::copyFrom(const Buffer& src)
{
    assert(src.bufferSize >= bufferSize);
    copyBuffer(
        *device,
        *buffer, *src.buffer,
        0u, 0u, bufferSize
    );
}


void vkb::Buffer::copyTo(const Buffer& dst)
{
    assert(dst.bufferSize >= bufferSize);
    copyBuffer(
        *device,
        *dst.buffer, *buffer,
        0u, 0u, bufferSize
    );
}

void vkb::Buffer::copyFrom(vk::DeviceSize size, const void* data, BufferRegion dstRegion)
{
    auto buf = map(dstRegion);
    memcpy(buf, data, size);
    unmap();
}


// --------------------------------- //
//        Device local buffer        //
// --------------------------------- //

vkb::DeviceLocalBuffer::DeviceLocalBuffer(
    vk::DeviceSize bufferSize,
    const void* data,
    vk::BufferUsageFlags usage,
    MemoryAllocator allocator)
    :
    DeviceLocalBuffer(
        vkb::VulkanBase::getDevice(),
        bufferSize,
        data,
        usage,
        std::move(allocator)
    )
{
}

vkb::DeviceLocalBuffer::DeviceLocalBuffer(
    const Device& device,
    vk::DeviceSize bufferSize,
    const void* data,
    vk::BufferUsageFlags usage,
    MemoryAllocator allocator)
    :
    Buffer(
        device,
        bufferSize,
        usage | vk::BufferUsageFlagBits::eTransferDst,
        vk::MemoryPropertyFlagBits::eDeviceLocal,
        std::move(allocator)
    )
{
    assert(bufferSize > 0);
    assert(data != nullptr);

    Buffer staging(
        device,
        bufferSize, data,
        vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostCoherent | vk::MemoryPropertyFlagBits::eHostVisible
    );

    copyFrom(staging);
}



// ------------------------------ //
//        Helper functions        //
// ------------------------------ //

void vkb::copyBuffer(
    const Device& device,
    const vk::Buffer& dst, const vk::Buffer& src,
    vk::DeviceSize dstOffset, vk::DeviceSize srcOffset, vk::DeviceSize size)
{
    auto transferBuffer = device.createTransferCommandBuffer();

    transferBuffer->begin({ vk::CommandBufferUsageFlagBits::eOneTimeSubmit });
    transferBuffer->copyBuffer(src, dst, vk::BufferCopy(srcOffset, dstOffset, size));
    transferBuffer->end();

    device.executeTransferCommandBufferSyncronously(*transferBuffer);
}
