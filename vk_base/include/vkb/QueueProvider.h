#pragma once

#include <vector>
#include <array>

#include <vulkan/vulkan.hpp>

#include "basics/Device.h"

namespace vkb
{

using phys_device_properties::familyIndex;
using phys_device_properties::queue_type;

constexpr size_t numQueueTypes_v = static_cast<size_t>(queue_type::numQueueTypes);

/*
Manages queues and provides helper methods to find the
appropriate queue for a given task.
Provides one queue for each queue capability. */
class QueueProvider
{
public:
    explicit QueueProvider(const Device& device);

    /**
     * Returns the primary queue of the specified type.
     */
    auto getQueue(queue_type type) const noexcept -> const vk::Queue&;

    /**
     * Returns the family index of the primary queue of the specified type.
     */
    auto getQueueFamilyIndex(queue_type type) const noexcept -> familyIndex;

    size_t getQueueFamilyCount() const noexcept;
    auto getQueueFamilies() const noexcept -> const std::vector<phys_device_properties::QueueFamily>&;

private:
    auto findQueues(const Device& device) const
        -> std::vector<std::pair<phys_device_properties::QueueFamily, std::vector<vk::Queue>>>;

    /**
     * Stores all queues with a certain capability at that
     * capability's index.
     */
    std::array<std::vector<vk::Queue>, numQueueTypes_v> queuesPerCapability{};

    /* All available queue families at their index */
    std::vector<phys_device_properties::QueueFamily> queueFamilies;

    /* One primary queue for each type/function. */
    std::array<vk::Queue, numQueueTypes_v> primaryQueues{};
    /* The families of the corresponding primary queues. */
    std::array<familyIndex, numQueueTypes_v> primaryQueueFamilies{};
};

} // namespace vkb
