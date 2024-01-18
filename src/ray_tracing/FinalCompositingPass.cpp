#include "trc/ray_tracing/FinalCompositingPass.h"

#include "trc/base/Barriers.h"

#include "trc/DescriptorSetUtils.h"
#include "trc/PipelineDefinitions.h"
#include "trc/RayShaders.h"
#include "trc/core/ComputePipelineBuilder.h"
#include "trc/core/RenderTarget.h"
#include "trc/ray_tracing/RayPipelineBuilder.h"
#include "trc/util/GlmStructuredBindings.h"



trc::rt::FinalCompositingPass::FinalCompositingPass(
    const Device& device,
    const RenderTarget& target,
    const FrameSpecific<RayBuffer>& rayBuffer)
    :
    RenderPass({}, NUM_SUBPASSES),
    device(device),
    renderTarget(&target),
    computeGroupSize(uvec3(
        glm::ceil(vec2(target.getSize()) / vec2(COMPUTE_LOCAL_SIZE)),
        1
    )),
    pool([&] {
        const ui32 frameCount{ target.getFrameClock().getFrameCount() };
        std::vector<vk::DescriptorPoolSize> poolSizes{
            // RayBuffer images
            { vk::DescriptorType::eStorageImage, RayBuffer::NUM_IMAGES * frameCount },
            // Swapchain output image
            { vk::DescriptorType::eStorageImage, frameCount },
        };
        return device->createDescriptorPoolUnique({
            vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
            // Two distinct sets: one for inputs, one for output
            2 * frameCount,
            poolSizes
        });
    }()),
    // Layouts
    inputLayout(buildDescriptorSetLayout()
        // Ray-Buffer bindings
        .addBinding(vk::DescriptorType::eStorageImage, 1,
                    vk::ShaderStageFlagBits::eCompute | ALL_RAY_PIPELINE_STAGE_FLAGS)
        .build(device)
    ),
    outputLayout(buildDescriptorSetLayout()
        .addBinding(vk::DescriptorType::eStorageImage, 1, vk::ShaderStageFlagBits::eCompute)
        .build(device)
    ),

    inputSets(target.getFrameClock()),
    outputSets(target.getFrameClock()),

    computePipelineLayout(device, { *inputLayout, *outputLayout, }, {}),
    computePipeline(buildComputePipeline()
        .setProgram(internal::loadShader(rt::shaders::getFinalCompositing()))
        .build(device, computePipelineLayout)
    )
{
    inputSets = {
        target.getFrameClock(),
        [&](ui32 i) -> vk::UniqueDescriptorSet {
            auto set = std::move(
                device->allocateDescriptorSetsUnique({ *pool, *inputLayout })[0]
            );

            auto& r = rayBuffer.getAt(i);
            std::vector<vk::DescriptorImageInfo> rayImages{
                { {}, r.getImageView(RayBuffer::eReflections), vk::ImageLayout::eGeneral },
            };

            std::vector<vk::WriteDescriptorSet> writes{
                { *set, 0, 0, vk::DescriptorType::eStorageImage, rayImages },
            };
            device->updateDescriptorSets(writes, {});

            return set;
        }
    };
    outputSets = {
        target.getFrameClock(),
        [&](ui32 i) -> vk::UniqueDescriptorSet {
            auto set = std::move(
                device->allocateDescriptorSetsUnique({ *pool, *outputLayout })[0]
            );

            vk::DescriptorImageInfo imageInfo({}, target.getImageView(i), vk::ImageLayout::eGeneral);
            vk::WriteDescriptorSet write(*set, 0, 0, vk::DescriptorType::eStorageImage, imageInfo);
            device->updateDescriptorSets(write, {});

            return set;
        }
    };

    inputSetProvider = std::make_shared<FrameSpecificDescriptorProvider>(inputSets);
    outputSetProvider = std::make_shared<FrameSpecificDescriptorProvider>(outputSets);

    computePipelineLayout.addStaticDescriptorSet(0, inputSetProvider);
    computePipelineLayout.addStaticDescriptorSet(1, outputSetProvider);
}

void trc::rt::FinalCompositingPass::begin(
    vk::CommandBuffer cmdBuf,
    vk::SubpassContents,
    FrameRenderState&)
{
    // Swapchain image: ePresentSrcKHR -> eGeneral
    imageMemoryBarrier(
        cmdBuf,
        renderTarget->getCurrentImage(),
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
        renderTarget->getCurrentImage(),
        vk::ImageLayout::eGeneral,
        vk::ImageLayout::ePresentSrcKHR,
        vk::PipelineStageFlagBits::eComputeShader,
        vk::PipelineStageFlagBits::eAllCommands,
        vk::AccessFlagBits::eShaderWrite,
        vk::AccessFlagBits::eHostRead,
        vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1)
    );
}

void trc::rt::FinalCompositingPass::end(vk::CommandBuffer)
{
}

void trc::rt::FinalCompositingPass::setRenderTarget(const RenderTarget& target)
{
    renderTarget = &target;
    for (ui32 i = 0; i < outputSets.getFrameClock().getFrameCount(); ++i)
    {
        auto set = *outputSets.getAt(i);

        vk::DescriptorImageInfo imageInfo({}, target.getImageView(i), vk::ImageLayout::eGeneral);
        vk::WriteDescriptorSet write(set, 0, 0, vk::DescriptorType::eStorageImage, imageInfo);
        device->updateDescriptorSets(write, {});
    }
}
