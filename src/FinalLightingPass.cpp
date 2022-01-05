#include "FinalLightingPass.h"

#include <vkb/ShaderProgram.h>

#include "core/PipelineLayoutBuilder.h"
#include "core/ComputePipelineBuilder.h"
#include "core/RenderTarget.h"
#include "DeferredRenderConfig.h"



trc::FinalLightingPass::FinalLightingPass(
    const vkb::Device& device,
    const RenderTarget& target,
    uvec2 offset,
    uvec2 size,
    DeferredRenderConfig& config)
    :
    RenderPass({}, 0),
    renderTarget(&target),
    renderConfig(&config),
    descSets(target.getFrameClock()),
    provider([&]() -> FrameSpecificDescriptorProvider {
        createDescriptors(device, target.getFrameClock());
        return { *descLayout, descSets };
    }()),
    layout(buildPipelineLayout()
        .addDescriptor(DescriptorName{ DeferredRenderConfig::GLOBAL_DATA_DESCRIPTOR }, true)
        .addDescriptor(DescriptorName{ DeferredRenderConfig::ASSET_DESCRIPTOR }, true)
        .addDescriptor(DescriptorName{ DeferredRenderConfig::G_BUFFER_DESCRIPTOR }, true)
        .addDescriptor(provider, true)
        .addDescriptor(DescriptorName{ DeferredRenderConfig::SCENE_DESCRIPTOR }, true)
        .addDescriptor(DescriptorName{ DeferredRenderConfig::SHADOW_DESCRIPTOR }, true)
        .addPushConstantRange({ vk::ShaderStageFlagBits::eCompute, 0, sizeof(vec2) * 2 })
        .build(device, config)
    ),
    pipeline(buildComputePipeline()
        .setProgram(vkb::readFile(TRC_SHADER_DIR"/final_lighting.comp.spv"))
        .build(device, layout)
    )
{
    setTargetArea(offset, size);
    setRenderTarget(device, target);
}

void trc::FinalLightingPass::begin(vk::CommandBuffer cmdBuf, vk::SubpassContents)
{
    vk::Image targetImage = renderTarget->getCurrentImage();

    pipeline.bind(cmdBuf, *renderConfig);
    cmdBuf.pushConstants<vec2>(*layout, vk::ShaderStageFlagBits::eCompute,
                               0, { renderOffset, renderSize });

    cmdBuf.pipelineBarrier(
        vk::PipelineStageFlagBits::eAllCommands,
        vk::PipelineStageFlagBits::eComputeShader,
        vk::DependencyFlagBits::eByRegion,
        {}, {},
        vk::ImageMemoryBarrier(
            {},
            vk::AccessFlagBits::eMemoryWrite,
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::eGeneral,
            VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
            targetImage,
            vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1)
        )
    );

    cmdBuf.dispatch(groupCount.x, groupCount.y, groupCount.z);

    cmdBuf.pipelineBarrier(
        vk::PipelineStageFlagBits::eComputeShader,
        vk::PipelineStageFlagBits::eAllCommands,
        vk::DependencyFlagBits::eByRegion,
        {}, {},
        vk::ImageMemoryBarrier(
            vk::AccessFlagBits::eShaderWrite,
            vk::AccessFlagBits::eMemoryRead,
            vk::ImageLayout::eGeneral,
            vk::ImageLayout::ePresentSrcKHR,
            VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
            targetImage,
            vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1)
        )
    );
}

void trc::FinalLightingPass::setTargetArea(uvec2 offset, uvec2 size)
{
    constexpr glm::uvec3 LOCAL_GROUP_SIZE{ 16, 16, 1 };

    renderOffset = vec2(offset);
    renderSize = vec2(size);
    groupCount = (uvec3(size, 1) + LOCAL_GROUP_SIZE - 1u) / LOCAL_GROUP_SIZE;
}

void trc::FinalLightingPass::setRenderTarget(const vkb::Device& device, const RenderTarget& target)
{
    renderTarget = &target;
    updateDescriptors(device, target);
}

void trc::FinalLightingPass::createDescriptors(
    const vkb::Device& device,
    const vkb::FrameClock& frameClock)
{
    const ui32 numSets{ frameClock.getFrameCount() };

    // Pool
    vk::DescriptorPoolSize poolSizes[]{
        { vk::DescriptorType::eStorageImage, numSets },
    };
    descPool = device->createDescriptorPoolUnique({
        vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet, numSets, poolSizes[0]
    });

    // Layout
    vk::DescriptorSetLayoutBinding layoutBindings[]{
        { 0, vk::DescriptorType::eStorageImage, 1, vk::ShaderStageFlagBits::eCompute }
    };
    descLayout = device->createDescriptorSetLayoutUnique(vk::DescriptorSetLayoutCreateInfo(
        {}, // vk::DescriptorSetLayoutCreateFlagBits::ePushDescriptorKHR
        layoutBindings[0]
    ));

    // Sets
    std::vector<vk::DescriptorSetLayout> layouts(numSets, *descLayout);
    descSets = { frameClock, device->allocateDescriptorSetsUnique({ *descPool, layouts }) };
}

void trc::FinalLightingPass::updateDescriptors(
    const vkb::Device& device,
    const RenderTarget& target)
{
    for (ui32 i = 0; auto view : target.getImageViews())
    {
        vk::DescriptorImageInfo imageInfo({}, view, vk::ImageLayout::eGeneral);
        vk::WriteDescriptorSet write(
            *descSets.getAt(i++), 0, 0, vk::DescriptorType::eStorageImage, imageInfo
        );

        device->updateDescriptorSets(write, {});
    }
}
