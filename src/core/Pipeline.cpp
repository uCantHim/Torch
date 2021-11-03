#include "Pipeline.h"



trc::Pipeline::Pipeline(
    vk::UniquePipelineLayout layout,
    vk::UniquePipeline pipeline,
    vk::PipelineBindPoint bindPoint)
    :
    layout(std::move(layout)),
    pipelineStorage(std::move(pipeline)),
    pipeline(*std::get<vk::UniquePipeline>(pipelineStorage)),
    bindPoint(bindPoint)
{}

trc::Pipeline::Pipeline(
    vk::UniquePipelineLayout layout,
    UniquePipelineStorageType pipeline,
    vk::PipelineBindPoint bindPoint)
    :
    layout(std::move(layout)),
    pipelineStorage(std::move(pipeline)),
    bindPoint(bindPoint)
{
    using UniquePipelineDl = vk::UniqueHandle<vk::Pipeline, vk::DispatchLoaderDynamic>;

    if (std::holds_alternative<vk::UniquePipeline>(pipelineStorage)) {
        this->pipeline = *std::get<vk::UniquePipeline>(pipelineStorage);
    }
    else if (std::holds_alternative<UniquePipelineDl>(pipelineStorage)) {
        this->pipeline = *std::get<UniquePipelineDl>(pipelineStorage);
    }
}

auto trc::Pipeline::operator*() const noexcept -> vk::Pipeline
{
    return pipeline;
}

auto trc::Pipeline::get() const noexcept -> vk::Pipeline
{
    return pipeline;
}

void trc::Pipeline::bind(vk::CommandBuffer cmdBuf) const
{
    cmdBuf.bindPipeline(bindPoint, pipeline);
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



auto trc::makeComputePipeline(
    const vkb::Device& device,
    vk::UniquePipelineLayout layout,
    vk::UniqueShaderModule shader,
    vk::PipelineCreateFlags flags,
    const std::string& entryPoint) -> Pipeline
{
    auto pipeline = device->createComputePipelineUnique(
        {},
        vk::ComputePipelineCreateInfo(
            flags,
            vk::PipelineShaderStageCreateInfo(
                {}, vk::ShaderStageFlagBits::eCompute, *shader, entryPoint.c_str()
            ),
            *layout
        )
    ).value;

    return Pipeline(std::move(layout), std::move(pipeline), vk::PipelineBindPoint::eCompute);
}
