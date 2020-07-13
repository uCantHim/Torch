#pragma once

#include "VulkanBase.h"

namespace vkb
{
    struct DeviceMemoryInternals
    {
        DeviceMemoryInternals(vk::DeviceMemory memory,
                              vk::DeviceSize size,
                              vk::DeviceSize offset = 0)
            : memory(memory), size(size), baseOffset(offset)
        {}

        vk::DeviceMemory memory;
        vk::DeviceSize size;             // Size of the memory
        vk::DeviceSize baseOffset{ 0 };  // Offset in pool-like structures
    };

    using DeviceMemoryDeleter = std::function<void(const DeviceMemoryInternals&)>;

    /**
     * @brief Frees the memory with a Vulkan device
     */
    class ClassicalDeviceMemoryDeleter
    {
    public:
        explicit ClassicalDeviceMemoryDeleter(const Device& device)
            : device(&device)
        {}

        void operator()(const DeviceMemoryInternals& internals) {
            device->get().freeMemory(internals.memory);
        }

    private:
        const Device* device;
    };

    /**
     * @brief Frees device memory with the default VulkanBase device
     */
    struct DefaultDeviceMemoryDeleter : public ClassicalDeviceMemoryDeleter
    {
    public:
        DefaultDeviceMemoryDeleter()
            : ClassicalDeviceMemoryDeleter(vkb::VulkanBase::getDevice())
        {}
    };

    /**
     * @brief Device Memory
     */
    class DeviceMemory
    {
    public:
        template<typename Del = DefaultDeviceMemoryDeleter>
        explicit DeviceMemory(DeviceMemoryInternals data)
            : DeviceMemory(std::move(data), Del())
        {}

        DeviceMemory(DeviceMemoryInternals data, DeviceMemoryDeleter deleter);

        DeviceMemory(const DeviceMemory&) = delete;
        DeviceMemory(DeviceMemory&&) noexcept;
        ~DeviceMemory();

        auto operator=(const DeviceMemory&) -> DeviceMemory& = delete;
        auto operator=(DeviceMemory&&) noexcept -> DeviceMemory&;

        /**
         * @brief Allocate a single piece of device memory
         *
         * This implements a canonical way of allocation device memory.
         *
         * @param const Device&           device       The device used to allocate the memory
         * @param vk::MemoryPropertyFlags properties   Properties that the type of the allocated
         *                                             memory should have
         * @param vk::MemoryRequirements  requirements
         *
         * @return DeviceMemory
         */
        static auto allocate(const Device& device,
                             vk::MemoryPropertyFlags properties,
                             vk::MemoryRequirements requirements) -> DeviceMemory;

        /**
         * @brief Bind the memory to a buffer
         */
        void bindToBuffer(const Device& device, vk::Buffer buffer);

        /**
         * @brief Map device memory to host memory
         *
         * Attention: Only one DeviceMemory from the same ManagedMemoryChunk
         * may be mapped at any time.
         *
         * Must not be called on memory that is already mapped.
         *
         * @param DeviceSize mappedSize Size of the mapped memory in bytes.
         * Must be less than or equal to the total memory size. May be
         * VK_WHOLE_SIZE.
         * @param DeviceSize offset     Offset in bytes from the start of the
         * memory.
         *
         * @return void* Pointer to mapped memory
         */
        auto map(const Device& device, vk::DeviceSize mappedOffset, vk::DeviceSize mappedSize) -> void*;

        /**
         * @brief Unmap the memory from host memory.
         *
         * Must not be called on memory that is not currently mapped.
         */
        void unmap(const Device& device);

        auto getSize() const noexcept -> vk::DeviceSize;

    private:
        bool isMovedFrom{ false };
        DeviceMemoryDeleter deleter;
        DeviceMemoryInternals internal;
    };
} // namespace vkb
