#include "DescriptorProvider.h"



trc::DescriptorProvider::DescriptorProvider(
    vk::DescriptorSetLayout layout,
    vk::DescriptorSet set)
    :
    layout(layout),
    set(set)
{
}

auto trc::DescriptorProvider::getDescriptorSet() const noexcept -> vk::DescriptorSet
{
    return set;
}

auto trc::DescriptorProvider::getDescriptorSetLayout() const noexcept -> vk::DescriptorSetLayout
{
    return layout;
}
