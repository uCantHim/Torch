#pragma once

#include <concepts>
#include <stdexcept>
#include <vector>

#include "PhysicalDevice.h"
#include "QueueManager.h"

namespace vkb
{

/**
 * A logical device used to interface with an underlying physical device.
 */
class Device
{
public:
    Device(const Device&) = delete;
    Device& operator=(const Device&) = delete;
    Device& operator=(Device&&) noexcept = delete;

    Device(Device&&) noexcept = default;
    ~Device() = default;

    /**
     * @brief Create a logical device from a physical device
     *
     * Fully initializes the device.
     *
     * Only pass a physical device to create a device with default
     * extensions and features.
     *
     * @param const PhysicalDevice& physDevice
     * @param std::vector<const char*> deviceExtensions Additional
     *        extensions to load on the device.
     * @param void* extraPhysicalDeviceFeatureChain Additional chained
     *        device features to enable on the logical device. This
     *        pointer will be set as pNext of the end of vkb's default
     *        feature chain.
     */
    explicit Device(const PhysicalDevice& physDevice,
                    std::vector<const char*> deviceExtensions = {},
                    void* extraPhysicalDeviceFeatureChain = nullptr);

    /**
     * @brief Create a device with a pre-created logical device
     *
     * Use this overload for maximum control over logical device creation.
     *
     * The constructor also has an overload that accepts various
     * configuration parameters and uses those to create a logical device.
     *
     * @param const PhysicalDevice& physDevice
     * @param vk::UniqueDevice device MUST have been created from the
     *                                physical device.
     */
    Device(const PhysicalDevice& physDevice, vk::UniqueDevice device);

    auto operator->() const noexcept -> const vk::Device*;
    auto operator*() const noexcept -> vk::Device;
    auto get() const noexcept -> vk::Device;

    auto getPhysicalDevice() const noexcept -> const PhysicalDevice&;

    auto getQueueManager() noexcept -> QueueManager&;
    auto getQueueManager() const noexcept -> const QueueManager&;

    /**
     * @brief Execute commands on the device
     *
     * Creates a transient command buffer, passes it to a function that
     * records some commands, then submits it to an appropriate queue.
     *
     * Only returns once all commands have finished executing.
     */
    template<std::invocable<vk::CommandBuffer> F>
    void executeCommands(QueueType queueType, F func) const;

private:
    const PhysicalDevice& physicalDevice;
    vk::UniqueDevice device;

    QueueManager queueManager;

    // Stores one command pool for each queue
    std::vector<vk::UniqueCommandPool> commandPools;
};

template<std::invocable<vk::CommandBuffer> F>
void Device::executeCommands(QueueType queueType, F func) const
{
    // Create a temporary fence
    auto fence = device->createFenceUnique({ vk::FenceCreateFlags() });

    // Get a suitable queue
    auto [queue, family] = queueManager.getAnyQueue(queueType);

    // Record commands
    auto cmdBuf = std::move(device->allocateCommandBuffersUnique({
        *commandPools[family], vk::CommandBufferLevel::ePrimary, 1
    })[0]);
    cmdBuf->begin(vk::CommandBufferBeginInfo{});
    func(*cmdBuf);
    cmdBuf->end();

    queue.waitSubmit(vk::SubmitInfo(0, nullptr, nullptr, 1, &*cmdBuf), *fence);

    // Wait for the fence
    if (device->waitForFences(*fence, true, UINT64_MAX) != vk::Result::eSuccess)
    {
        throw std::runtime_error(
            "Error while waiting for fence in Device::executeCommandsSynchronously"
        );
    }
}

} // namespace vkb
