#include "Device.h"

#include <set>
#include <iostream>



vkb::Device::Device(const PhysicalDevice& physDevice)
    :
    physicalDevice(physDevice),
    device(physDevice.createLogicalDevice()),
    graphicsPool(device->createCommandPoolUnique(
        {
            vk::CommandPoolCreateFlagBits::eResetCommandBuffer
            | vk::CommandPoolCreateFlagBits::eTransient,
            physDevice.queueCapabilities.graphicsCapable[0].index
        }
    )),
    graphicsQueue(device->getQueue(physDevice.queueCapabilities.graphicsCapable[0].index, 0)),
    transferPool(device->createCommandPoolUnique(
        {
            vk::CommandPoolCreateFlagBits::eResetCommandBuffer
            | vk::CommandPoolCreateFlagBits::eTransient,
            physDevice.queueCapabilities.transferCapable[0].index
        }
    )),
    transferQueue(device->getQueue(physDevice.queueCapabilities.transferCapable[0].index, 0))
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
