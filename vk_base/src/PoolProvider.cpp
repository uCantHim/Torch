#include "PoolProvider.h"



vkb::PoolProvider::PoolProvider(const Device& device, const QueueProvider& queueProvider)
    :
    device(device),
    queueProvider(queueProvider)
{
    // Create pools for each index
    for (familyIndex i = 0; i < static_cast<familyIndex>(queueProvider.getQueueFamilyCount()); i++)
    {
        auto standardPool = createPool(vk::CommandPoolCreateFlagBits::eResetCommandBuffer, i);
        auto transientPool = createPool(
            vk::CommandPoolCreateFlagBits::eTransient
            | vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
            i
        );
        pools.push_back({ std::move(standardPool), std::move(transientPool) });
    }
}


auto vkb::PoolProvider::createPool(vk::CommandPoolCreateFlags flags, uint32_t queueFamily) -> vk::UniqueCommandPool
{
    return device->createCommandPoolUnique(vk::CommandPoolCreateInfo(flags, queueFamily));
}


auto vkb::PoolProvider::getPool(pool_type type, familyIndex queueFamily) const noexcept -> const vk::CommandPool&
{
    return pools[queueFamily][static_cast<size_t>(type)].get();
}


auto vkb::PoolProvider::getPool(pool_type type, queue_type queueType) const noexcept -> const vk::CommandPool&
{
    return this->getPool(type, queueProvider.getQueueFamilyIndex(queueType));
}
