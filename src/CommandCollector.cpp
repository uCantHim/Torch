#include "CommandCollector.h"



trc::CommandCollector::CommandCollector()
    :
    pool(vkb::getDevice()->createCommandPoolUnique(
        vk::CommandPoolCreateInfo(
            vk::CommandPoolCreateFlagBits::eResetCommandBuffer
            | vk::CommandPoolCreateFlagBits::eTransient,
            vkb::getQueueProvider().getQueueFamilyIndex(vkb::QueueType::graphics)
        )
    ))
{
    commandBuffers.push_back(
        vkb::FrameSpecificObject<vk::UniqueCommandBuffer>(
            vkb::VulkanBase::getDevice()->allocateCommandBuffersUnique(
                { *pool, vk::CommandBufferLevel::ePrimary, vkb::getSwapchain().getFrameCount() }
            )
        )
    );
    commandBuffers.push_back(
        vkb::FrameSpecificObject<vk::UniqueCommandBuffer>(
            vkb::VulkanBase::getDevice()->allocateCommandBuffersUnique(
                { *pool, vk::CommandBufferLevel::ePrimary, vkb::getSwapchain().getFrameCount() }
            )
        )
    );
}

auto trc::CommandCollector::recordScene(
    SceneBase& scene,
    RenderStage::ID renderStage,
    RenderPass& renderPass) -> std::vector<vk::CommandBuffer>
{
    const RenderPass::ID renderPassId = renderPass.id();
    auto cmdBuf = **commandBuffers[renderStage];

    // Set up rendering
    cmdBuf.reset({});
    cmdBuf.begin({ vk::CommandBufferUsageFlagBits::eOneTimeSubmit });
    renderPass.begin(cmdBuf, vk::SubpassContents::eInline);

    // Record all commands
    const ui32 subPassCount = renderPass.getNumSubPasses();
    for (ui32 subPass = 0; subPass < subPassCount; subPass++)
    {
        for (auto pipeline : scene.getPipelines(renderStage, subPass))
        {
            // Bind the current pipeline
            auto& p = GraphicsPipeline::at(pipeline);
            p.bind(cmdBuf);
            p.bindStaticDescriptorSets(cmdBuf);

            // Record commands for all objects with this pipeline
            scene.invokeDrawFunctions(renderStage, renderPassId, subPass, pipeline, cmdBuf);
        }

        if (subPass < subPassCount - 1) {
            cmdBuf.nextSubpass(vk::SubpassContents::eInline);
        }
    }

    renderPass.end(cmdBuf);
    cmdBuf.end();

    return { cmdBuf };
}
