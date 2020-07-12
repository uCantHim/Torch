#include "Buffer.h"

#include "basics/Device.h"



// -------------------- //
//        Base buffer        //
// -------------------- //

vkb::Buffer::Buffer(
    vk::DeviceSize bufferSize,
    vk::BufferUsageFlags usage,
    vk::MemoryPropertyFlags flags)
    :
    buffer(getDevice()->createBufferUnique(
        vk::BufferCreateInfo{
            vk::BufferCreateFlags(),
            bufferSize,
            usage,
            vk::SharingMode::eExclusive,
            0,
            nullptr
        }
    )),
    memory([&] {
        auto physicalDevice = getDevice().getPhysicalDevice();
        auto memReq = getDevice()->getBufferMemoryRequirements(*buffer);
        return getDevice()->allocateMemoryUnique(
            { memReq.size, physicalDevice.findMemoryType(memReq.memoryTypeBits, flags) }
        );
    }()),
    bufferSize(bufferSize)
{
    getDevice()->bindBufferMemory(*buffer, *memory, 0);
}


auto vkb::Buffer::map(vk::DeviceSize offset, vk::DeviceSize size) -> memptr
{
    return static_cast<memptr>(getDevice()->mapMemory(*memory, offset, size));
}


void vkb::Buffer::unmap()
{
    getDevice()->unmapMemory(*memory);
}


void vkb::Buffer::copyFrom(const Buffer& src)
{
    assert(src.bufferSize >= bufferSize);
    copyBuffer(
        getDevice(),
        *buffer, *src.buffer,
        0u, 0u, bufferSize
    );
}


void vkb::Buffer::copyTo(const Buffer& dst)
{
    assert(dst.bufferSize >= bufferSize);
    copyBuffer(
        getDevice(),
        *dst.buffer, *buffer,
        0u, 0u, bufferSize
    );
}


// ---------------------------- //
//        Device local buffer        //
// ---------------------------- //

vkb::DeviceLocalBuffer::DeviceLocalBuffer(vk::DeviceSize size, const void* data, vk::BufferUsageFlags usage)
    :
    Buffer(size, usage | vk::BufferUsageFlagBits::eTransferDst, vk::MemoryPropertyFlagBits::eDeviceLocal)
{
    assert(size > 0);
    assert(data != nullptr);

    Buffer staging(
        size, vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostCoherent | vk::MemoryPropertyFlagBits::eHostVisible
    );

    void* stagingMem = staging.map();
    memcpy(stagingMem, data, size);
    staging.unmap();

    copyFrom(staging);
}


// ------------------------ //
//        Uniform Buffer        //
// ------------------------ //

vkb::UniformBuffer::UniformBuffer(vk::DeviceSize size)
    :
    buffers(
        [&](uint32_t) -> vkb::Buffer {
            return Buffer(
                size,
                vk::BufferUsageFlagBits::eUniformBuffer,
                vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
            );
        }
    )
{
}


auto vkb::UniformBuffer::get() const noexcept -> const vkb::Buffer&
{
    return buffers.get();
}



// ---------------------------- //
//        Helper functions        //
// ---------------------------- //

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
