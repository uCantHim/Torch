#pragma once

#include <vulkan/vulkan.hpp>

#include "FrameSpecificObject.h"

namespace vkb
{

using memptr = uint8_t*;

/**
 * @brief A buffer backed by managed device memory
 */
class Buffer : private vkb::VulkanBase
{
public:
    Buffer(vk::DeviceSize bufferSize, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags flags);
    Buffer(const Buffer&) = delete;
    Buffer(Buffer&&) = default;
    ~Buffer() noexcept = default;

    Buffer& operator=(const Buffer&) = delete;
    Buffer& operator=(Buffer&&) = default;

    auto inline operator*() const noexcept -> const vk::Buffer& {
        return *buffer;
    }

    auto inline operator->() const noexcept -> const vk::Buffer* {
        return &*buffer;
    }

    inline auto get() const noexcept -> const vk::Buffer& {
        return *buffer;
    }

    /**
     * @brief Map a range of the buffer to CPU memory
     *
     * Size may be VK_WHOLE_SIZE to map the whole buffer.
     *
     * @param vk::DeviceSize offset Offset into the buffer in bytes
     * @param vk::DeviceSize size     Number of bytes mapped starting
     *                                 at the specified offset. Must be
     *                                 less than or equal to the total
     *                                 buffer size.
     */
    auto map(vk::DeviceSize offset = 0, vk::DeviceSize size = VK_WHOLE_SIZE) -> memptr;
    void unmap();

    void copyFrom(const Buffer& src);
    void copyTo(const Buffer& dst);

private:
    vk::UniqueBuffer buffer;
    vk::UniqueDeviceMemory memory;

    vk::DeviceSize bufferSize{ 0 };
    mutable bool isMapped{ false };
};


/*
A buffer intended for transfer operations.
 - Host visible & coherent
 - Transfer dst & src */
class CopyBuffer : public Buffer
{
public:
    explicit CopyBuffer(vk::DeviceSize bufferSize)
        : Buffer(
            bufferSize,
            vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc,
            vk::MemoryPropertyFlagBits::eHostCoherent | vk::MemoryPropertyFlagBits::eHostVisible
        )
    {}
};


/*
A high-performance device-local buffer.
Cannot be copied to or from, cannot be mapped. Thus, must and can only
be assigned at creation time. */
class DeviceLocalBuffer : private Buffer
{
public:
    DeviceLocalBuffer(vk::DeviceSize size, void* data, vk::BufferUsageFlags usage);

    [[nodiscard]]
    inline auto get() const noexcept -> const vk::Buffer& {
        return Buffer::get();
    }
};


/*
A uniform buffer with standard settings:
 - One buffer per frame (FrameSpecificObject)
 - host coherent and host visible
 - uniform buffer usage */
class UniformBuffer : public VulkanBase
{
public:
    explicit UniformBuffer(vk::DeviceSize size);
    UniformBuffer(const UniformBuffer&) = delete;
    UniformBuffer(UniformBuffer&&) noexcept = default;
    ~UniformBuffer() = default;

    UniformBuffer& operator=(const UniformBuffer&) = delete;
    UniformBuffer& operator=(UniformBuffer&&) noexcept = default;

    [[nodiscard]]
    auto get() const noexcept -> const Buffer&;

private:
    FrameSpecificObject<Buffer> buffers;
};




// ---------------------------- //
//        Helper functions        //
// ---------------------------- //

void copyBuffer(
    const Device& device,
    const vk::Buffer& dst, const vk::Buffer& src,
    vk::DeviceSize dstOffset, vk::DeviceSize srcOffset, vk::DeviceSize size
);

} // namespace vkb
