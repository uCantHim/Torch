#pragma once

#include <vkb/Buffer.h>

#include "Types.h"

namespace trc
{
    template<typename T>
    struct DynamicBuffer
    {
    public:
        DynamicBuffer(const vkb::Device& device,
                      ui32 initialSize,
                      vk::BufferUsageFlags usageFlags,
                      vk::MemoryPropertyFlags memoryProperties,
                      const vkb::DeviceMemoryAllocator& alloc = vkb::DefaultDeviceMemoryAllocator{})
            :
            device(device),
            alloc(alloc),
            usageFlags(
                usageFlags
                | vk::BufferUsageFlagBits::eTransferSrc
                | vk::BufferUsageFlagBits::eTransferDst
            ),
            memoryProperties(memoryProperties),
            buffer(device, initialSize * sizeof(T), this->usageFlags, memoryProperties, alloc),
            mappedBuf(reinterpret_cast<T*>(buffer.map()))
        {
        }

        inline auto operator*() const -> vk::Buffer {
            return *buffer;
        }

        inline void push(T val)
        {
            if (currentOffset * sizeof(T) >= buffer.size()) {
                reserve((buffer.size() / sizeof(T)) * 2);
            }

            mappedBuf[currentOffset] = val;
            currentOffset++;
        }

        inline void clear()
        {
            currentOffset = 0;
        }

        inline auto size() const -> ui32
        {
            return currentOffset;
        }

        inline void reserve(ui32 newSize)
        {
            buffer.unmap();
            vkb::Buffer newBuffer(device, newSize * sizeof(T), usageFlags, memoryProperties, alloc);
            vkb::copyBuffer(device, *newBuffer, *buffer, 0, 0, buffer.size());
            buffer = std::move(newBuffer);
            mappedBuf = reinterpret_cast<T*>(buffer.map());
        }

    private:
        const vkb::Device& device;
        const vkb::DeviceMemoryAllocator alloc;
        const vk::BufferUsageFlags usageFlags;
        const vk::MemoryPropertyFlags memoryProperties;

        vkb::Buffer buffer;
        T* mappedBuf;
        ui32 currentOffset{ 0 };
    };
} // namespace trc
