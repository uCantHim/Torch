#include "trc/base/Buffer.h"

#include "trc/base/Barriers.h"



// ------------------------- //
//        Base buffer        //
// ------------------------- //

trc::Buffer::Buffer(
    const Device& device,
    vk::DeviceSize bufferSize,
    vk::BufferUsageFlags usage,
    vk::MemoryPropertyFlags flags,
    const DeviceMemoryAllocator& allocator)
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


trc::Buffer::Buffer(
    const Device& device,
    vk::DeviceSize bufferSize,
    const void* data,
    vk::BufferUsageFlags usage,
    vk::MemoryPropertyFlags flags,
    const DeviceMemoryAllocator& allocator)
    :
    Buffer(device, bufferSize, usage, flags, allocator)
{
    copyFrom(bufferSize, data);
}


auto trc::Buffer::getMemory() const -> const DeviceMemory&
{
    return memory;
}


auto trc::Buffer::size() const noexcept -> vk::DeviceSize
{
    return bufferSize;
}


auto trc::Buffer::map(vk::DeviceSize offset, vk::DeviceSize size) const -> uint8_t*
{
    return static_cast<uint8_t*>(memory.map(*device, offset, size));
}


void trc::Buffer::unmap() const
{
    memory.unmap(*device);
}


void trc::Buffer::flush(vk::DeviceSize offset, vk::DeviceSize size) const
{
    memory.flush(*device, offset, size);
}


void trc::Buffer::barrier(
    vk::CommandBuffer cmdBuf,
    vk::DeviceSize offset,
    vk::DeviceSize size,
    vk::PipelineStageFlags srcStages,
    vk::PipelineStageFlags dstStages,
    vk::AccessFlags srcAccess,
    vk::AccessFlags dstAccess) const
{
    bufferMemoryBarrier(cmdBuf, *buffer, offset, size, srcStages, dstStages, srcAccess, dstAccess);
}


void trc::Buffer::copyFrom(const Buffer& src, BufferRegion srcRegion, vk::DeviceSize dstOffset)
{
    if (srcRegion.size == VK_WHOLE_SIZE) {
        srcRegion.size = src.bufferSize;
    }

    copyBuffer(
        *device,
        *buffer, *src.buffer,
        dstOffset, srcRegion.offset, srcRegion.size
    );
}


void trc::Buffer::copyTo(const Buffer& dst, BufferRegion srcRegion, vk::DeviceSize dstOffset)
{
    if (srcRegion.size == VK_WHOLE_SIZE) {
        srcRegion.size = bufferSize;
    }

    copyBuffer(
        *device,
        *dst.buffer, *buffer,
        dstOffset, srcRegion.offset, srcRegion.size
    );
}

void trc::Buffer::copyFrom(vk::DeviceSize size, const void* data, BufferRegion dstRegion)
{
    if (data != nullptr)
    {
        auto buf = map(dstRegion.offset, dstRegion.size);
        memcpy(buf, data, size);
        unmap();
    }
}


// --------------------------------- //
//        Device local buffer        //
// --------------------------------- //

trc::DeviceLocalBuffer::DeviceLocalBuffer(
    const Device& device,
    vk::DeviceSize bufferSize,
    const void* data,
    vk::BufferUsageFlags usage,
    const DeviceMemoryAllocator& allocator)
    :
    Buffer(
        device,
        bufferSize,
        usage | vk::BufferUsageFlagBits::eTransferDst,
        vk::MemoryPropertyFlagBits::eDeviceLocal,
        allocator
    )
{
    assert(bufferSize > 0);

    if (data != nullptr)
    {
        Buffer staging(
            device,
            bufferSize, data,
            vk::BufferUsageFlagBits::eTransferSrc,
            vk::MemoryPropertyFlagBits::eHostCoherent | vk::MemoryPropertyFlagBits::eHostVisible
        );

        copyFrom(staging);
    }
}



// ------------------------------ //
//        Helper functions        //
// ------------------------------ //

void trc::copyBuffer(
    const Device& device,
    const vk::Buffer& dst, const vk::Buffer& src,
    vk::DeviceSize dstOffset, vk::DeviceSize srcOffset, vk::DeviceSize size)
{
    device.executeCommands(QueueType::transfer, [&](auto cmdBuf) {
        cmdBuf.copyBuffer(src, dst, vk::BufferCopy(srcOffset, dstOffset, size));
    });
}
