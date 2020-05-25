#pragma once

#include <vulkan/vulkan.hpp>

#include "QueueProvider.h"

namespace vkb
{

using phys_device_properties::familyIndex;
using phys_device_properties::queue_type;

enum class pool_type {
    standard,
    transient,
    numPoolTypes
};
constexpr size_t numPoolTypes_v = static_cast<size_t>(pool_type::numPoolTypes);


/*
Provides command pools.
Currently provides two command pools for each queue family:
 - A 'standard' command pool that is used for persistent command buffers
 - A command pool for short-lived command buffers only. */
class PoolProvider
{
public:
    PoolProvider(const Device& device, const QueueProvider& queueProvider);

    /**
     * Creates a standalone pool that is completely unrelated to the PoolProvider.
     */
    auto createPool(vk::CommandPoolCreateFlags flags, familyIndex queueFamily = 0u) -> vk::UniqueCommandPool;

    /**
     * Returns a pool of the specified type used by the specified queue family.
     */
    auto getPool(pool_type type, familyIndex queueFamily) const noexcept -> const vk::CommandPool&;

    /**
     * Returns a pool of the specified type used by the queue family of the
     * primary queue of the specififed type.
     */
    auto getPool(pool_type type, queue_type queueType) const noexcept -> const vk::CommandPool&;

private:
    const Device& device;
    const QueueProvider& queueProvider;

    std::vector<std::array<vk::UniqueCommandPool, numPoolTypes_v>> pools;
};

} // namespace vkb
