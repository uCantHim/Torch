#include "Pipeline.h"



trc::Pipeline::Pipeline(
    vk::UniquePipelineLayout layout,
    vk::UniquePipeline pipeline,
    vk::PipelineBindPoint bindPoint)
    :
    layout(std::move(layout)),
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
        provider->bindDescriptorSet(cmdBuf, bindPoint, *layout, index);
    }
}

void trc::Pipeline::bindDefaultPushConstantValues(vk::CommandBuffer cmdBuf) const
{
    for (const auto& [offset, stages, data] : defaultPushConstants)
    {
        cmdBuf.pushConstants(*layout, stages, offset, data.size(), data.data());
    }
}

auto trc::Pipeline::getLayout() const noexcept -> vk::PipelineLayout
{
    return *layout;
}

void trc::Pipeline::addStaticDescriptorSet(
    ui32 descriptorIndex,
    const DescriptorProviderInterface& provider) noexcept
{
    staticDescriptorSets.emplace_back(descriptorIndex, &provider);
}



auto trc::makeGraphicsPipeline(
    ui32 index,
    vk::UniquePipelineLayout layout,
    vk::UniquePipeline pipeline) -> Pipeline&
{
    return Pipeline::replace(
        index,
        std::move(layout),
        std::move(pipeline),
        vk::PipelineBindPoint::eGraphics
    );
}

auto trc::makeGraphicsPipeline(
    ui32 index,
    vk::UniquePipelineLayout layout,
    const vk::GraphicsPipelineCreateInfo& info
    ) -> Pipeline&
{
    return Pipeline::replace(
        index,
        std::move(layout),
        vkb::VulkanBase::getDevice()->createGraphicsPipelineUnique({}, info).value,
        vk::PipelineBindPoint::eGraphics
    );
}

auto trc::makeComputePipeline(
    ui32 index,
    vk::UniquePipelineLayout layout,
    vk::UniquePipeline pipeline
    ) -> Pipeline&
{
    return Pipeline::replace(
        index,
        std::move(layout),
        std::move(pipeline),
        vk::PipelineBindPoint::eCompute
    );
}

auto trc::makeComputePipeline(
    ui32 index,
    vk::UniquePipelineLayout layout,
    const vk::ComputePipelineCreateInfo& info
    ) -> Pipeline&
{
    return Pipeline::replace(
        index,
        std::move(layout),
        vkb::getDevice()->createComputePipelineUnique({}, info).value,
        vk::PipelineBindPoint::eCompute
    );
}
