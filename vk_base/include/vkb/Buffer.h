#pragma once

#include "VulkanInclude.h"

#include "Memory.h"

namespace vkb
{
    struct BufferRegion
    {
        BufferRegion() = default;
        BufferRegion(vk::DeviceSize offset, vk::DeviceSize size)
            : offset(offset), size(size) {}

        vk::DeviceSize offset{ 0 };
        vk::DeviceSize size{ VK_WHOLE_SIZE };
    };

    /**
     * @brief A buffer backed by managed device memory
     */
    class Buffer
    {
    public:
        Buffer() = default;

        Buffer(const Device& device,
               vk::DeviceSize bufferSize,
               vk::BufferUsageFlags usage,
               vk::MemoryPropertyFlags flags,
               const DeviceMemoryAllocator& allocator = DefaultDeviceMemoryAllocator());

        Buffer(const Device& device,
               vk::DeviceSize bufferSize,
               const void* data,
               vk::BufferUsageFlags usage,
               vk::MemoryPropertyFlags flags,
               const DeviceMemoryAllocator& allocator = DefaultDeviceMemoryAllocator());

        template<typename T>
        Buffer(const Device& device,
               const std::vector<T>& data,
               vk::BufferUsageFlags usage,
               vk::MemoryPropertyFlags flags,
               const DeviceMemoryAllocator& allocator = DefaultDeviceMemoryAllocator())
            : Buffer(device, sizeof(T) * data.size(), data.data(), usage, flags, allocator)
        {}

        Buffer(const Buffer&) = delete;
        Buffer(Buffer&&) = default;
        ~Buffer() noexcept = default;

        Buffer& operator=(const Buffer&) = delete;
        Buffer& operator=(Buffer&&) = default;

        auto inline operator*() const noexcept -> vk::Buffer {
            return *buffer;
        }

        inline auto get() const noexcept -> vk::Buffer {
            return *buffer;
        }

        auto getMemory() const -> const DeviceMemory&;

        auto size() const noexcept -> vk::DeviceSize;

        /**
         * @brief Map a range of the buffer to CPU memory
         *
         * Size may be VK_WHOLE_SIZE to map the whole buffer.
         *
         * @param vk::DeviceSize offset Offset into the buffer in bytes
         * @param vk::DeviceSize size   Number of bytes mapped starting at the
         *                              specified offset. Must be less than or
         *                              equal to the total buffer size.
         */
        auto map(vk::DeviceSize offset = 0, vk::DeviceSize size = VK_WHOLE_SIZE) const -> uint8_t*;

        /**
         * @brief Map a range of the buffer to CPU memory
         *
         * Size may be VK_WHOLE_SIZE to map the whole buffer.
         *
         * @tparam T A pointer type. Returns a pointer of this type.
         *
         * @param vk::DeviceSize offset Offset into the buffer in bytes
         * @param vk::DeviceSize size   Number of bytes mapped starting at the
         *                              specified offset. Must be less than or
         *                              equal to the total buffer size.
         */
        template<typename T> requires std::is_pointer_v<T>
        auto map(vk::DeviceSize offset = 0, vk::DeviceSize size = VK_WHOLE_SIZE) const -> T
        {
            return reinterpret_cast<T>(map(offset, size));
        }

        /**
         * @brief Unmap the buffer if its memory is currently host mapped
         */
        void unmap() const;

        void flush(vk::DeviceSize offset = 0, vk::DeviceSize size = VK_WHOLE_SIZE) const;

        void barrier(vk::CommandBuffer cmdBuf,
                     vk::DeviceSize offset,
                     vk::DeviceSize size,
                     vk::PipelineStageFlags srcStages,
                     vk::PipelineStageFlags dstStages,
                     vk::AccessFlags srcAccess,
                     vk::AccessFlags dstAccess) const;

        void copyFrom(const Buffer& src, BufferRegion srcRegion = {}, vk::DeviceSize dstOffset = 0);
        void copyTo(const Buffer& dst, BufferRegion srcRegion = {}, vk::DeviceSize dstOffset = 0);

        void copyFrom(vk::DeviceSize copySize, const void* data, BufferRegion dstRegion = {});

        template<typename T>
        inline void copyFrom(const T& data, BufferRegion dstRegion) {
            copyFrom(sizeof(T), &data, dstRegion);
        }

        template<typename T>
        inline void copyFrom(const std::vector<T>& data, BufferRegion dstRegion) {
            copyFrom(sizeof(T) * data.size(), data.data(), dstRegion);
        }

    private:
        const Device* device{ nullptr };

        vk::UniqueBuffer buffer;
        DeviceMemory memory;

        vk::DeviceSize bufferSize{ 0 };
    };


    /**
     * A buffer intended for transfer operations.
     *  - Host visible & coherent
     *  - Transfer dst & src
     */
    class CopyBuffer : public Buffer
    {
    public:
        CopyBuffer(const Device& device, vk::DeviceSize bufferSize)
            : Buffer(
                device,
                bufferSize,
                vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc,
                vk::MemoryPropertyFlagBits::eHostCoherent | vk::MemoryPropertyFlagBits::eHostVisible
            )
        {}
    };


    /**
     * A high-performance device-local buffer.
     * Cannot be copied to or from, cannot be mapped. Thus, must and can only
     * be assigned at creation time.
     */
    class DeviceLocalBuffer : private Buffer
    {
    public:
        using Buffer::barrier;

        DeviceLocalBuffer() = default;

        DeviceLocalBuffer(const Device& device,
                          vk::DeviceSize bufferSize,
                          const void* data,
                          vk::BufferUsageFlags usage,
                          const DeviceMemoryAllocator& allocator = DefaultDeviceMemoryAllocator());

        template<typename T>
        DeviceLocalBuffer(const Device& device,
                          const std::vector<T>& data,
                          vk::BufferUsageFlags usage,
                          const DeviceMemoryAllocator& allocator = DefaultDeviceMemoryAllocator())
            : DeviceLocalBuffer(device, sizeof(T) * data.size(), data.data(), usage, allocator)
        {}

        inline auto operator*() const noexcept -> vk::Buffer {
            return Buffer::get();
        }

        inline auto get() const noexcept -> vk::Buffer {
            return Buffer::get();
        }
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
