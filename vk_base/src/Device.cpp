#include "Device.h"

#include <set>
#include <iostream>

#include "VulkanDebug.h"



vkb::Device::Device(
    const PhysicalDevice& physDevice,
    std::vector<const char*> deviceExtensions,
    void* extraPhysicalDeviceFeatureChain)
    :
    Device(
        physDevice,
        physDevice.createLogicalDevice(
            std::move(deviceExtensions),
            extraPhysicalDeviceFeatureChain
        )
    )
{
}

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
            queueManager.getPrimaryQueueFamily(QueueType::graphics)
        }
    )),
    transferPool(device->createCommandPoolUnique(
        {
            vk::CommandPoolCreateFlagBits::eResetCommandBuffer
            | vk::CommandPoolCreateFlagBits::eTransient,
            queueManager.getPrimaryQueueFamily(QueueType::transfer)
        }
    ))
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

auto vkb::Device::createGraphicsCommandBuffer(vk::CommandBufferLevel level) const
    -> vk::UniqueCommandBuffer
{
    return std::move(device->allocateCommandBuffersUnique({ *graphicsPool, level, 1 })[0]);
}

void vkb::Device::executeGraphicsCommandBuffer(vk::CommandBuffer cmdBuf) const
{
    queueManager.getPrimaryQueue(QueueType::graphics).waitSubmit(
        vk::SubmitInfo(0, nullptr, nullptr, 1, &cmdBuf),
        {}
    );
}

void vkb::Device::executeGraphicsCommandBufferSynchronously(vk::CommandBuffer cmdBuf) const
{
    auto fence = device->createFenceUnique({ vk::FenceCreateFlags() });
    queueManager.getPrimaryQueue(QueueType::graphics).waitSubmit(
        vk::SubmitInfo(0, nullptr, nullptr, 1, &cmdBuf),
        *fence
    );

    if (device->waitForFences(*fence, true, UINT64_MAX) != vk::Result::eSuccess)
    {
        throw std::runtime_error(
            "[In Device::executeGraphicsCommandBufferSynchronously]: waitForFences did not return"
            " vk::Result::eSuccess when waiting for queue submission."
        );
    }
}

auto vkb::Device::createTransferCommandBuffer(vk::CommandBufferLevel level) const
    -> vk::UniqueCommandBuffer
{
    return std::move(device->allocateCommandBuffersUnique({ *transferPool, level, 1 })[0]);
}

void vkb::Device::executeTransferCommandBuffer(vk::CommandBuffer cmdBuf) const
{
    queueManager.getPrimaryQueue(QueueType::transfer).waitSubmit(
        vk::SubmitInfo(0, nullptr, nullptr, 1, &cmdBuf),
        {}
    );
}

void vkb::Device::executeTransferCommandBufferSyncronously(vk::CommandBuffer cmdBuf) const
{
    auto fence = device->createFenceUnique({ vk::FenceCreateFlags() });
    queueManager.getPrimaryQueue(QueueType::transfer).waitSubmit(
        vk::SubmitInfo(0, nullptr, nullptr, 1, &cmdBuf),
        *fence
    );

    if (device->waitForFences(*fence, true, UINT64_MAX) != vk::Result::eSuccess)
    {
        throw std::runtime_error(
            "[In Device::executeTransferCommandBufferSyncronously]: waitForFences did not return"
            " vk::Result::eSuccess when waiting for queue submission."
        );
    }
}
