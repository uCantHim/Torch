#pragma once

#include <vector>
#include <array>

#include <vulkan/vulkan.hpp>

#include "basics/Device.h"

namespace vkb
{
    using phys_device_properties::QueueFamilyIndex;
    using phys_device_properties::QueueType;

    constexpr size_t numQueueTypes_v = static_cast<size_t>(QueueType::numQueueTypes);

    /**
     * Manages queues and provides helper methods to find the
     * appropriate queue for a given task.
     * Provides one queue for each queue capability.
     */
    class QueueProvider
    {
    public:
        explicit QueueProvider(const Device& device);

        /**
         * Returns the first queue of the specified type.
         */
        auto getQueue(QueueType type) const noexcept -> vk::Queue;

        /**
         * Returns the family index of the primary queue of the specified type.
         */
        auto getQueueFamilyIndex(QueueType type) const noexcept -> QueueFamilyIndex;

    private:
        auto findQueues(const Device& device) const
            -> std::vector<std::pair<phys_device_properties::QueueFamily, std::vector<vk::Queue>>>;

        /**
         * Stores all queues with a certain capability at that
         * capability's index.
         */
        std::array<std::vector<vk::Queue>, numQueueTypes_v> queuesPerCapability{};

        /** All available queue families at their index */
        std::vector<phys_device_properties::QueueFamily> queueFamilies;

        /** One primary queue for each type/function. */
        std::array<vk::Queue, numQueueTypes_v> primaryQueues{};
        /** The families of the corresponding primary queues. */
        std::array<QueueFamilyIndex, numQueueTypes_v> primaryQueueFamilies{};
    };
} // namespace vkb
