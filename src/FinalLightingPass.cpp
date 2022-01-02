#include "FinalLightingPass.h"

#include <vkb/ShaderProgram.h>

#include "core/Window.h"
#include "core/PipelineLayoutBuilder.h"
#include "core/ComputePipelineBuilder.h"
#include "DeferredRenderConfig.h"



trc::FinalLightingPass::FinalLightingPass(const Window& window, DeferredRenderConfig& config)
    :
    RenderPass({}, 0),
    window(&window),
    layout(buildPipelineLayout()
        .addDescriptor(DescriptorName{ DeferredRenderConfig::GLOBAL_DATA_DESCRIPTOR }, true)
        .addDescriptor(DescriptorName{ DeferredRenderConfig::ASSET_DESCRIPTOR }, true)
        .addDescriptor(DescriptorName{ DeferredRenderConfig::G_BUFFER_DESCRIPTOR }, true)
        .addDescriptor(DescriptorName{ DeferredRenderConfig::SCENE_DESCRIPTOR }, true)
        .addDescriptor(DescriptorName{ DeferredRenderConfig::SHADOW_DESCRIPTOR }, true)
        .build(window.getInstance(), config)
    ),
    pipeline(buildComputePipeline()
        .setProgram(vkb::readFile(TRC_SHADER_DIR"/final_lighting.comp.spv"))
        .build(window.getDevice(), layout)
    )
{
    resize(window.getSize());
}

void trc::FinalLightingPass::begin(vk::CommandBuffer cmdBuf, vk::SubpassContents)
{
    vk::Image swapchainImage = window->getImage(window->getCurrentFrame());
    pipeline.bind(cmdBuf);

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
            swapchainImage,
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
            swapchainImage,
            vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1)
        )
    );
}

void trc::FinalLightingPass::resize(uvec2 windowSize)
{
    constexpr glm::uvec3 LOCAL_GROUP_SIZE{ 16, 16, 1 };

    groupCount = (uvec3(windowSize, 1) + LOCAL_GROUP_SIZE - 1u) / LOCAL_GROUP_SIZE;
}
