#include "trc/base/Device.h"



trc::Device::Device(
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

trc::Device::Device(
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

auto trc::Device::operator->() const noexcept -> const vk::Device*
{
    return &*device;
}

auto trc::Device::operator*() const noexcept -> vk::Device
{
    return *device;
}

auto trc::Device::get() const noexcept -> vk::Device
{
    return *device;
}

auto trc::Device::getPhysicalDevice() const noexcept -> const PhysicalDevice&
{
    return physicalDevice;
}

auto trc::Device::getQueueManager() noexcept -> QueueManager&
{
    return queueManager;
}

auto trc::Device::getQueueManager() const noexcept -> const QueueManager&
{
    return queueManager;
}
