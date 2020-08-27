#include "Pipeline.h"



auto trc::PipelineLayout::operator*() const noexcept -> vk::PipelineLayout
{
    return *layout;
}

auto trc::PipelineLayout::get() const noexcept -> vk::PipelineLayout
{
    return *layout;
}



trc::Pipeline::Pipeline(
    vk::PipelineLayout layout,
    vk::UniquePipeline pipeline,
    vk::PipelineBindPoint bindPoint)
    :
    layout(layout),
    pipeline(std::move(pipeline)),
    bindPoint(bindPoint)
{}

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



auto trc::makeGraphicsPipeline(
    ui32 index,
    vk::PipelineLayout layout,
    vk::UniquePipeline pipeline) -> Pipeline&
{
    return Pipeline::emplace(
        index,
        layout,
        std::move(pipeline),
        vk::PipelineBindPoint::eGraphics
    );
}

auto trc::makeGraphicsPipeline(
    ui32 index,
    vk::PipelineLayout layout,
    const vk::GraphicsPipelineCreateInfo& info
    ) -> Pipeline&
{
    return Pipeline::emplace(
        index,
        layout,
        vkb::VulkanBase::getDevice()->createGraphicsPipelineUnique({}, info).value,
        vk::PipelineBindPoint::eGraphics
    );
}

auto trc::makeComputePipeline(
    ui32 index,
    vk::PipelineLayout layout,
    vk::UniquePipeline pipeline
    ) -> Pipeline&
{
    return Pipeline::emplace(
        index,
        layout,
        std::move(pipeline),
        vk::PipelineBindPoint::eCompute
    );
}

auto trc::makeComputePipeline(
    ui32 index,
    vk::PipelineLayout layout,
    const vk::ComputePipelineCreateInfo& info
    ) -> Pipeline&
{
    return Pipeline::emplace(
        index,
        layout,
        vkb::getDevice()->createComputePipelineUnique({}, info).value,
        vk::PipelineBindPoint::eCompute
    );
}
