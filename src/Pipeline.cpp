#include "Pipeline.h"



auto trc::PipelineLayout::operator*() const noexcept -> vk::PipelineLayout
{
    return *layout;
}

auto trc::PipelineLayout::get() const noexcept -> vk::PipelineLayout
{
    return *layout;
}



auto trc::Pipeline::operator*() const noexcept -> vk::Pipeline
{
    return *pipeline;
}

auto trc::Pipeline::get() const noexcept -> vk::Pipeline
{
    return *pipeline;
}

void trc::Pipeline::bind(vk::CommandBuffer cmdBuf) const
{
    cmdBuf.bindPipeline(vk::PipelineBindPoint::eGraphics, *pipeline);
}

void trc::Pipeline::bindStaticDescriptorSets(vk::CommandBuffer cmdBuf) const
{
    for (const auto& [index, provider] : staticDescriptorSets)
    {
        cmdBuf.bindDescriptorSets(bindPoint, layout, index, provider->getDescriptorSet(), {});
    }
}

auto trc::Pipeline::getLayout() const noexcept -> vk::PipelineLayout
{
    return layout;
}

void trc::Pipeline::addStaticDescriptorSet(
    ui32 descriptorIndex,
    const DescriptorProviderInterface& provider) noexcept
{
    staticDescriptorSets.emplace_back(descriptorIndex, &provider);
}
