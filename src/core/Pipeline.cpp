#include "Pipeline.h"



trc::Pipeline::Pipeline(
    PipelineLayout& layout,
    vk::UniquePipeline pipeline,
    vk::PipelineBindPoint bindPoint)
    :
    layout(&layout),
    pipelineStorage(std::move(pipeline)),
    pipeline(*std::get<vk::UniquePipeline>(pipelineStorage)),
    bindPoint(bindPoint)
{
    if (!layout) {
        throw Exception("[In Pipeline::Pipeline]: Specified layout is not a valid layout handle");
    }
}

trc::Pipeline::Pipeline(
    PipelineLayout& layout,
    UniquePipelineHandleType pipeline,
    vk::PipelineBindPoint bindPoint)
    :
    layout(&layout),
    pipelineStorage(std::move(pipeline)),
    bindPoint(bindPoint)
{
    if (!layout) {
        throw Exception("[In Pipeline::Pipeline]: Specified layout is not a valid layout handle");
    }

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
    assert(*layout);

    cmdBuf.bindPipeline(bindPoint, pipeline);
    layout->bindStaticDescriptorSets(cmdBuf, bindPoint);
    layout->bindDefaultPushConstantValues(cmdBuf);
}

auto trc::Pipeline::getLayout() noexcept -> PipelineLayout&
{
    assert(layout != nullptr);
    return *layout;
}

auto trc::Pipeline::getLayout() const noexcept -> const PipelineLayout&
{
    assert(layout != nullptr);
    return *layout;
}



auto trc::makeComputePipeline(
    const vkb::Device& device,
    PipelineLayout& layout,
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

    return Pipeline(layout, std::move(pipeline), vk::PipelineBindPoint::eCompute);
}
