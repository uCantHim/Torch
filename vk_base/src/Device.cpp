#include "Device.h"



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
    queueManager(physDevice, *this)
{
    // Create one command pool for each queue family. These will be used
    // for the executeCommands* functions.
    commandPools.resize(physicalDevice.queueFamilies.size());
    for (auto& family : physicalDevice.queueFamilies)
    {
        commandPools[family.index] = device->createCommandPoolUnique({
            vk::CommandPoolCreateFlagBits::eResetCommandBuffer
            | vk::CommandPoolCreateFlagBits::eTransient,
            family.index
        });
    }
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
