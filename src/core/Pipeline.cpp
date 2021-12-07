#include "Pipeline.h"



trc::Pipeline::Pipeline(
    PipelineLayout layout,
    vk::UniquePipeline pipeline,
    vk::PipelineBindPoint bindPoint)
    :
    layout(std::move(layout)),
    pipelineStorage(std::move(pipeline)),
    pipeline(*std::get<vk::UniquePipeline>(pipelineStorage)),
    bindPoint(bindPoint)
{}

trc::Pipeline::Pipeline(
    PipelineLayout layout,
    UniquePipelineHandleType pipeline,
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
    layout.bindStaticDescriptorSets(cmdBuf, bindPoint);
    layout.bindDefaultPushConstantValues(cmdBuf);
}

auto trc::Pipeline::getLayout() noexcept -> PipelineLayout&
{
    return layout;
}

auto trc::Pipeline::getLayout() const noexcept -> const PipelineLayout&
{
    return layout;
}



auto trc::makeComputePipeline(
    const vkb::Device& device,
    PipelineLayout layout,
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
