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

void trc::Pipeline::bindDescriptorSets(vk::CommandBuffer cmdBuf) const
{
    for (const auto& [index, set] : staticDescriptorSets)
    {
        cmdBuf.bindDescriptorSets(bindPoint, layout, index, set, {});
    }
}

auto trc::Pipeline::getLayout() const noexcept -> vk::PipelineLayout
{
    return layout;
}

void trc::Pipeline::addStaticDescriptorSet(ui32 descriptorIndex, vk::DescriptorSet set) noexcept
{
    staticDescriptorSets.emplace_back(descriptorIndex, set);
}
