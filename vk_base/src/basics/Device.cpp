#include "Device.h"

#include <set>
#include <iostream>

#include "VulkanDebug.h"



vkb::Device::Device(
    const PhysicalDevice& physDevice,
    vk::UniqueDevice logicalDevice)
    :
    physicalDevice(physDevice),
    device(std::move(logicalDevice)),
    queueManager(physDevice, *this),
    // Retrieve all queues from the device
    graphicsPool(device->createCommandPoolUnique(
        {
            vk::CommandPoolCreateFlagBits::eResetCommandBuffer
            | vk::CommandPoolCreateFlagBits::eTransient,
            getQueueFamily(QueueType::graphics)
        }
    )),
    graphicsQueue(getQueue(QueueType::graphics)),
    transferPool(device->createCommandPoolUnique(
        {
            vk::CommandPoolCreateFlagBits::eResetCommandBuffer
            | vk::CommandPoolCreateFlagBits::eTransient,
            getQueueFamily(QueueType::transfer)
        }
    )),
    transferQueue(getQueues(getQueueFamily(QueueType::transfer)).at(1))
{
}

auto vkb::Device::operator->() const noexcept -> const vk::Device*
{
    return &*device;
}

auto vkb::Device::operator*() const noexcept -> vk::Device
{
    return *device;
}

auto vkb::Device::get() const noexcept -> vk::Device
{
    return *device;
}

auto vkb::Device::getPhysicalDevice() const noexcept -> const PhysicalDevice&
{
    return physicalDevice;
}

auto vkb::Device::getQueueManager() noexcept -> QueueManager&
{
    return queueManager;
}

auto vkb::Device::getQueueManager() const noexcept -> const QueueManager&
{
    return queueManager;
}

auto vkb::Device::getQueue(QueueType capability, uint32_t queueIndex) const -> vk::Queue
{
    return queueManager.getPrimaryQueue(capability, queueIndex);
}

auto vkb::Device::getQueueFamily(QueueType capability) const -> QueueFamilyIndex
{
    return queueManager.getPrimaryQueueFamily(capability);
}

auto vkb::Device::getQueues(QueueFamilyIndex family) const -> std::vector<vk::Queue>
{
    return queueManager.getFamilyQueues(family);
}

auto vkb::Device::createGraphicsCommandBuffer(vk::CommandBufferLevel level) const
    -> vk::UniqueCommandBuffer
{
    return std::move(device->allocateCommandBuffersUnique({ *graphicsPool, level, 1 })[0]);
}

void vkb::Device::executeGraphicsCommandBuffer(vk::CommandBuffer cmdBuf) const
{
    graphicsQueue.submit(vk::SubmitInfo(0, nullptr, nullptr, 1, &cmdBuf), {});
}

void vkb::Device::executeGraphicsCommandBufferSynchronously(vk::CommandBuffer cmdBuf) const
{
    auto fence = device->createFenceUnique({ vk::FenceCreateFlags() });
    graphicsQueue.submit(
        vk::SubmitInfo(0, nullptr, nullptr, 1, &cmdBuf),
        *fence
    );
    assert(device->waitForFences(*fence, true, UINT64_MAX) == vk::Result::eSuccess);
}

auto vkb::Device::createTransferCommandBuffer(vk::CommandBufferLevel level) const
    -> vk::UniqueCommandBuffer
{
    return std::move(device->allocateCommandBuffersUnique({ *transferPool, level, 1 })[0]);
}

void vkb::Device::executeTransferCommandBuffer(vk::CommandBuffer cmdBuf) const
{
    transferQueue.submit(vk::SubmitInfo(0, nullptr, nullptr, 1, &cmdBuf), {});
}

void vkb::Device::executeTransferCommandBufferSyncronously(vk::CommandBuffer cmdBuf) const
{
    auto fence = device->createFenceUnique({ vk::FenceCreateFlags() });
    transferQueue.submit(
        vk::SubmitInfo(0, nullptr, nullptr, 1, &cmdBuf),
        *fence
    );
    assert(device->waitForFences(*fence, true, UINT64_MAX) == vk::Result::eSuccess);
}
