#include "PipelineLayout.h"

#include "DescriptorProvider.h"



trc::PipelineLayout::PipelineLayout(vk::UniquePipelineLayout layout)
    :
    layout(std::move(layout))
{
}

trc::PipelineLayout::PipelineLayout(
    const vkb::Device& device,
    const std::vector<vk::DescriptorSetLayout>& descriptors,
    const std::vector<vk::PushConstantRange>& pushConstants)
    :
    layout(device->createPipelineLayoutUnique({ {}, descriptors, pushConstants }))
{

}

auto trc::PipelineLayout::operator*() const noexcept -> vk::PipelineLayout
{
    return *layout;
}

void trc::PipelineLayout::bindStaticDescriptorSets(
    vk::CommandBuffer cmdBuf,
    vk::PipelineBindPoint bindPoint) const
{
    for (const auto& [index, provider] : staticDescriptorSets)
    {
        provider->bindDescriptorSet(cmdBuf, bindPoint, *layout, index);
    }
}

void trc::PipelineLayout::bindDefaultPushConstantValues(vk::CommandBuffer cmdBuf) const
{
    for (const auto& [offset, stages, data] : defaultPushConstants)
    {
        cmdBuf.pushConstants(*layout, stages, offset, data.size(), data.data());
    }
}

void trc::PipelineLayout::addStaticDescriptorSet(
    ui32 descriptorIndex,
    const DescriptorProviderInterface& provider) noexcept
{
    staticDescriptorSets.emplace_back(descriptorIndex, &provider);
}
