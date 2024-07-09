#include "trc/ray_tracing/FinalCompositingPass.h"

#include "trc/DescriptorSetUtils.h"
#include "trc/PipelineDefinitions.h"
#include "trc/RayShaders.h"
#include "trc/TorchRenderStages.h"
#include "trc/base/Barriers.h"
#include "trc/core/ComputePipelineBuilder.h"
#include "trc/core/DeviceTask.h"
#include "trc/util/GlmStructuredBindings.h"



namespace trc::rt
{

CompositingDescriptor::CompositingDescriptor(const Device& device, ui32 maxDescriptorSets)
{
    auto builder = buildDescriptorSetLayout()
        .addBinding(vk::DescriptorType::eStorageImage, 1, vk::ShaderStageFlagBits::eCompute)
        .addBinding(vk::DescriptorType::eStorageImage, 1, vk::ShaderStageFlagBits::eCompute);

    pool = builder.buildPool(device, maxDescriptorSets,
                             vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet);
    layout = builder.build(device);
}

auto CompositingDescriptor::makeDescriptorSet(
    const Device& device,
    const RayBuffer& rayBuffer,
    vk::ImageView outputImage) -> vk::UniqueDescriptorSet
{
    auto set = std::move(device->allocateDescriptorSetsUnique({ *pool, *layout })[0]);

    vk::DescriptorImageInfo dstImage({}, outputImage, vk::ImageLayout::eGeneral);
    std::vector<vk::DescriptorImageInfo> rayImages{
        { {}, rayBuffer.getImageView(RayBuffer::eReflections), vk::ImageLayout::eGeneral },
    };

    std::vector<vk::WriteDescriptorSet> writes{
        { *set, 0, 0, vk::DescriptorType::eStorageImage, dstImage },
        { *set, 1, 0, vk::DescriptorType::eStorageImage, rayImages },
    };
    device->updateDescriptorSets(writes, {});

    return set;
}

auto CompositingDescriptor::getDescriptorSetLayout() const -> vk::DescriptorSetLayout
{
    return *layout;
}



FinalCompositingDispatcher::FinalCompositingDispatcher(
    const Device& device,
    const RayBuffer& rayBuffer,
    const Viewport& renderTarget,
    CompositingDescriptor& descriptor)
    :
    device(device),
    computeGroupSize(uvec3(
        glm::ceil(vec2(renderTarget.area.size) / vec2(COMPUTE_LOCAL_SIZE)),
        1
    )),
    descSet(descriptor.makeDescriptorSet(device, rayBuffer, renderTarget.target.imageView)),
    computePipelineLayout(device, descriptor.getDescriptorSetLayout(), {}),
    computePipeline(buildComputePipeline()
        .setProgram(internal::loadShader(rt::shaders::getFinalCompositing()))
        .build(device, computePipelineLayout)
    ),
    descriptorProvider(std::make_shared<DescriptorProvider>(*descSet)),
    targetImage(renderTarget.target.image)
{
    computePipelineLayout.addStaticDescriptorSet(0, descriptorProvider);
}

void FinalCompositingDispatcher::createTasks(ViewportDrawTaskQueue& taskQueue)
{
    taskQueue.spawnTask(
        finalCompositingRenderStage,
        [this](vk::CommandBuffer cmdBuf, ViewportDrawContext&)
        {
            // Swapchain image: ePresentSrcKHR -> eGeneral
            imageMemoryBarrier(
                cmdBuf,
                targetImage,
                vk::ImageLayout::ePresentSrcKHR,
                vk::ImageLayout::eGeneral,
                vk::PipelineStageFlagBits::eComputeShader,
                vk::PipelineStageFlagBits::eComputeShader,
                vk::AccessFlagBits::eShaderWrite,
                vk::AccessFlagBits::eShaderWrite | vk::AccessFlagBits::eShaderRead,
                vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1)
            );

            computePipeline.bind(cmdBuf);
            auto [x, y, z] = computeGroupSize;
            cmdBuf.dispatch(x, y, z);

            // Swapchain image: eGeneral -> ePresentSrcKHR
            imageMemoryBarrier(
                cmdBuf,
                targetImage,
                vk::ImageLayout::eGeneral,
                vk::ImageLayout::ePresentSrcKHR,
                vk::PipelineStageFlagBits::eComputeShader,
                vk::PipelineStageFlagBits::eAllCommands,
                vk::AccessFlagBits::eShaderWrite,
                vk::AccessFlagBits::eHostRead,
                vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1)
            );
        }
    );
}

} // namespace trc::rt
