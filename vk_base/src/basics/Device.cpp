#include "Device.h"

#include <set>
#include <iostream>

#include "VulkanDebug.h"



namespace vkb
{
    /**
     * @return nullopt if no queue with the requested capability exists.
     */
    auto findMostSpecialized(QueueType type, const std::vector<QueueFamily>& families)
        -> std::optional<QueueFamilyIndex>
    {
        // Queue families with the number of additional capabilities
        std::vector<std::pair<QueueFamilyIndex, uint32_t>> possibleFamilies;

        for (const auto& family : families)
        {
            if (family.isCapable(type))
            {
                auto& [_, numCapabilities] = possibleFamilies.emplace_back(family.index, 0);

                // Test for other supported capabilities
                for (uint32_t i = 0; i < static_cast<uint32_t>(QueueType::numQueueTypes); i++)
                {
                    if (family.isCapable(static_cast<QueueType>(i))){
                        numCapabilities++;
                    }
                }
            }
        }

        if (possibleFamilies.empty()) {
            return std::nullopt;
        }

        // Find family with the least number of capabilities
        uint32_t minCapabilities = 6;
        QueueFamilyIndex familyWithMinCapabilities{ UINT32_MAX };
        for (const auto& [familyIndex, capabilities] : possibleFamilies)
        {
            if (capabilities < minCapabilities) {
                familyWithMinCapabilities = familyIndex;
            }
        }

        if (familyWithMinCapabilities == UINT32_MAX) {
            return std::nullopt;
        }
        return familyWithMinCapabilities;
    }
} // namespace vkb



vkb::Device::Device(const PhysicalDevice& physDevice)
    :
    physicalDevice(physDevice),
    device(physDevice.createLogicalDevice()),
    // Retrieve all queues from the device
    queuesPerFamily([&]() {
        std::vector<std::vector<vk::Queue>> result(physDevice.queueFamilies.size());
        for (const auto& family : physDevice.queueFamilies)
        {
            auto& queues = result.at(family.index);
            for (uint32_t i = 0; i < family.queueCount; i++)
            {
                queues.push_back(device->getQueue(family.index, i));
            }
        }
        return result;
    }()),
    // Sort queue families by most specialized
    mostSpecializedQueueFamilies([&]() {
        std::array<QueueFamilyIndex, queueTypeCount> result;
        for (size_t i = 0; i < queueTypeCount; i++)
        {
            auto mostSpecialized = findMostSpecialized(
                static_cast<QueueType>(i),
                physDevice.queueFamilies);

            if (mostSpecialized.has_value()) {
                result[i] = mostSpecialized.value();
            }
            else {
                result[i] = UINT32_MAX;
            }
        }
        return result;
    }()),
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
    if constexpr (enableVerboseLogging)
    {
        std::cout << "\n";
        for (int i = 0; i < static_cast<int>(QueueType::numQueueTypes); i++)
        {
            if (mostSpecializedQueueFamilies[i] != UINT32_MAX)
            {
                std::cout << "--- Chose queue family " << mostSpecializedQueueFamilies[i]
                    << " as the primary " << std::to_string(QueueType(i)) << " queue family.\n";
            }
            else
            {
                std::cout << "--- No queue family found with " << std::to_string(QueueType(i))
                    << " support.\n";
            }
        }
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

auto vkb::Device::getQueue(QueueType capability, uint32_t queueIndex) -> vk::Queue
{
    return queuesPerFamily.at(getQueueFamily(capability)).at(queueIndex);
}

auto vkb::Device::getQueueFamily(QueueType capability) -> QueueFamilyIndex
{
    QueueFamilyIndex family = mostSpecializedQueueFamilies[static_cast<size_t>(capability)];
    if (family == UINT32_MAX)
    {
        throw std::out_of_range("[Device::getQueueFamily]: No queue supports the requested "
                                "capability" + std::to_string(static_cast<size_t>(capability)));
    }

    return family;
}

auto vkb::Device::getQueues(QueueFamilyIndex family) -> const std::vector<vk::Queue>&
{
    return queuesPerFamily.at(family);
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
