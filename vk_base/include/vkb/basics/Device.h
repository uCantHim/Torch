#pragma once

#include <vector>

#include "PhysicalDevice.h"

namespace vkb
{

/**
 * A logical device used to interface with an underlying physical device.
 */
class Device
{
public:
    /**
     * @brief Create a logical device from a physical device
     *
     * Fully initializes the device.
     *
     * Use this constructor overload and pass only a physical device to
     * create a device with default extensions and features.
     *
     * @tparam ...DeviceFeatures Additional device feature types that shall
     *                           be enabled on the device.
     *
     * @param const PhysicalDevice& physDevice
     * @param std::vector<const char*> deviceExtensions Additional
     *        extensions to load on the device.
     * @param std::tuple<DeviceFeatures...> One cannot specify constructor
     *        template arguments explicitly, so we have to use this proxy
     *        parameter.
     */
    template<typename... DeviceFeatures>
    explicit Device(const PhysicalDevice& physDevice,
                    std::vector<const char*> deviceExtensions = {},
                    std::tuple<DeviceFeatures...> = {});

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

    Device(Device&&) noexcept = default;

    Device(const Device&) = delete;
    Device& operator=(const Device&) = delete;
    Device& operator=(Device&&) noexcept = delete;

    auto operator->() const noexcept -> const vk::Device*;
    auto operator*() const noexcept -> vk::Device;
    auto get() const noexcept -> vk::Device;

    auto getPhysicalDevice() const noexcept -> const PhysicalDevice&;

    /**
     * @param QueueType capability
     * @param uint32_t  queueIndex
     *
     * @return vk::Queue The queueIndex-th queue of the most specialized
     *                   queue family for the requested type.
     *
     * @throw std::out_of_range if no queue with the index exists.
     */
    auto getQueue(QueueType capability, uint32_t queueIndex = 0) const -> vk::Queue;

    /**
     * @return vk::Queue The most specialized queue family for the
     *                   requested type.
     */
    auto getQueueFamily(QueueType capability) const -> QueueFamilyIndex;

    /**
     * @return All queues of a specific family
     */
    auto getQueues(QueueFamilyIndex family) const -> const std::vector<vk::Queue>&;

    /**
     * @brief Create a temporary command buffer for graphics operations
     *
     * Allocates the command buffer from a pool with the reset and the
     * transient flags set.
     *
     * The command buffer CAN only be executed on a queue of the first
     * graphics-capable queue family. To ensure that this is the case, you
     * can use Device::executeGraphicsCommandBuffer(), which submits a
     * command buffer to that specific queue.
     *
     * @param vk::CommandBufferLevel level Create a primary or a secondary
     *                                     command buffer.
     *
     * @return vk::UniqueCommandBuffer A command buffer
     */
    auto createGraphicsCommandBuffer(vk::CommandBufferLevel level = vk::CommandBufferLevel::ePrimary) const
        -> vk::UniqueCommandBuffer;

    /**
     * @brief Execute a command buffer
     *
     * Executes a command buffer on the first queue of the first
     * graphics-capable queue family.
     *
     * @param vk::CommandBuffer cmdBuf The command buffer to execute
     */
    void executeGraphicsCommandBuffer(vk::CommandBuffer cmdBuf) const;

    /**
     * @brief Execute a command buffer synchronously
     *
     * Creates a fence and waits for it to be signaled before the function
     * returns.
     *
     * Executes the command buffer on the first queue of the first
     * graphics-capable queue family.
     *
     * @param vk::CommandBuffer cmdBuf The command buffer to execute
     */
    void executeGraphicsCommandBufferSynchronously(vk::CommandBuffer cmdBuf) const;

    /**
     * @brief Create a temporary command buffer for transfer operations
     *
     * Allocates the command buffer from a pool with the reset and the
     * transient flags set.
     *
     * The command buffer CAN only be executed on a queue of the first
     * transfer-capable queue family. To ensure that this is the case, you
     * can use Device::executeTransferCommandBuffer(), which submits a
     * command buffer to that specific queue.
     *
     * @param vk::CommandBufferLevel level Create a primary or a secondary
     *                                     command buffer.
     *
     * @return vk::UniqueCommandBuffer A command buffer
     */
    auto createTransferCommandBuffer(vk::CommandBufferLevel level = vk::CommandBufferLevel::ePrimary) const
        -> vk::UniqueCommandBuffer;

    /**
     * @brief Execute a command buffer
     *
     * Executes a command buffer on the first queue of the first
     * transfer-capable queue family.
     *
     * @param vk::CommandBuffer cmdBuf The command buffer to execute
     */
    void executeTransferCommandBuffer(vk::CommandBuffer cmdBuf) const;

    /**
     * @brief Execute a command buffer synchronously
     *
     * Creates a fence and waits for it to be signaled before the function
     * returns.
     *
     * Executes the command buffer on the first queue of the first
     * transfer-capable queue family.
     *
     * @param vk::CommandBuffer cmdBuf The command buffer to execute
     */
    void executeTransferCommandBufferSyncronously(vk::CommandBuffer cmdBuf) const;

    template<std::invocable<vk::CommandBuffer> F>
    void executeCommandsSynchronously(QueueType queueType, F func);

private:
    const PhysicalDevice& physicalDevice;
    vk::UniqueDevice device;

    static constexpr size_t queueTypeCount = static_cast<size_t>(QueueType::numQueueTypes);
    std::vector<std::vector<vk::Queue>> queuesPerFamily;
    std::array<QueueFamilyIndex, queueTypeCount> mostSpecializedQueueFamilies;

    vk::UniqueCommandPool graphicsPool;
    vk::Queue graphicsQueue;
    vk::UniqueCommandPool transferPool;
    vk::Queue transferQueue;
};



template<typename... DeviceFeatures>
Device::Device(
    const PhysicalDevice& physDevice,
    std::vector<const char*> deviceExtensions,
    std::tuple<DeviceFeatures...>)
    :
    Device(
        physDevice,
        physDevice.createLogicalDevice<DeviceFeatures...>(std::move(deviceExtensions))
    )
{
}

template<std::invocable<vk::CommandBuffer> F>
void Device::executeCommandsSynchronously(QueueType queueType, F func)
{
    auto pool = device->createCommandPoolUnique(vk::CommandPoolCreateInfo(
        vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
        getQueueFamily(queueType)
    ));

    auto cmdBuf = std::move(device->allocateCommandBuffersUnique({
        *pool,
        vk::CommandBufferLevel::ePrimary,
        1
    })[0]);
    cmdBuf->begin(vk::CommandBufferBeginInfo{});
    func(*cmdBuf);
    cmdBuf->end();

    auto fence = device->createFenceUnique({ vk::FenceCreateFlags() });
    getQueue(queueType, 0).submit(
        vk::SubmitInfo(0, nullptr, nullptr, 1, &*cmdBuf),
        *fence
    );
    if (device->waitForFences(*fence, true, UINT64_MAX) != vk::Result::eSuccess) {
        throw std::runtime_error("Failed to wait for fence in Device::executeCommandsSynchronously");
    }
}

} // namespace vkb
