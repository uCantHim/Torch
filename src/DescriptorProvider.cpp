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

void trc::DescriptorProvider::setDescriptorSet(vk::DescriptorSet newSet)
{
    set = newSet;
}

void trc::DescriptorProvider::setDescriptorSetLayout(vk::DescriptorSetLayout newLayout)
{
    layout = newLayout;
}



trc::FrameSpecificDescriptorProvider::FrameSpecificDescriptorProvider(
    vk::DescriptorSetLayout layout,
    vkb::FrameSpecificObject<vk::DescriptorSet> set)
    :
    layout(layout),
    set(std::move(set))
{
}

auto trc::FrameSpecificDescriptorProvider::getDescriptorSet() const noexcept -> vk::DescriptorSet
{
    return *set;
}

auto trc::FrameSpecificDescriptorProvider::getDescriptorSetLayout() const noexcept
    -> vk::DescriptorSetLayout
{
    return layout;
}

void trc::FrameSpecificDescriptorProvider::setDescriptorSet(
    vkb::FrameSpecificObject<vk::DescriptorSet> newSet)
{
    set = std::move(newSet);
}

void trc::FrameSpecificDescriptorProvider::setDescriptorSetLayout(
    vk::DescriptorSetLayout newLayout)
{
    layout = newLayout;
}
