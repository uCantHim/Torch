#pragma once

#include "trc/base/Buffer.h"

#include "trc/Types.h"

namespace trc
{
    template<typename T>
    struct DynamicBuffer
    {
    public:
        DynamicBuffer(const Device& device,
                      ui32 initialSize,
                      vk::BufferUsageFlags usageFlags,
                      vk::MemoryPropertyFlags memoryProperties,
                      const DeviceMemoryAllocator& alloc = DefaultDeviceMemoryAllocator{})
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
            Buffer newBuffer(device, newSize * sizeof(T), usageFlags, memoryProperties, alloc);
            copyBuffer(device, *newBuffer, *buffer, 0, 0, buffer.size());
            buffer = std::move(newBuffer);
            mappedBuf = reinterpret_cast<T*>(buffer.map());
        }

    private:
        const Device& device;
        const DeviceMemoryAllocator alloc;
        const vk::BufferUsageFlags usageFlags;
        const vk::MemoryPropertyFlags memoryProperties;

        Buffer buffer;
        T* mappedBuf;
        ui32 currentOffset{ 0 };
    };
} // namespace trc
