#include "CommandCollector.h"



trc::CommandCollector::CommandCollector()
    :
    pool(vkb::getDevice()->createCommandPoolUnique(
        vk::CommandPoolCreateInfo(
            vk::CommandPoolCreateFlagBits::eResetCommandBuffer
            | vk::CommandPoolCreateFlagBits::eTransient,
            vkb::getDevice().getQueueFamily(vkb::QueueType::graphics)
        )
    )),
    commandBuffers(
        vkb::VulkanBase::getDevice()->allocateCommandBuffersUnique(
            { *pool, vk::CommandBufferLevel::ePrimary, vkb::getSwapchain().getFrameCount() }
        )
    )
{
}

auto trc::CommandCollector::recordScene(
    SceneBase& scene,
    RenderStageType::ID stageType,
    const std::vector<RenderPass::ID>& passes
    ) -> vk::CommandBuffer
{
    auto cmdBuf = **commandBuffers;

    // Set up rendering
    cmdBuf.reset({});
    cmdBuf.begin({ vk::CommandBufferUsageFlagBits::eOneTimeSubmit });
    for (auto renderPassId : passes)
    {
        auto& renderPass = RenderPass::at(renderPassId);
        renderPass.begin(cmdBuf, vk::SubpassContents::eInline);

        // Record all commands
        const ui32 subPassCount = renderPass.getNumSubPasses();
        for (ui32 subPass = 0; subPass < subPassCount; subPass++)
        {
            for (auto pipeline : scene.getPipelines(stageType, subPass))
            {
                // Bind the current pipeline
                auto& p = Pipeline::at(pipeline);
                p.bind(cmdBuf);
                p.bindStaticDescriptorSets(cmdBuf);
                p.bindDefaultPushConstantValues(cmdBuf);

                // Record commands for all objects with this pipeline
                scene.invokeDrawFunctions(stageType, renderPassId, subPass, pipeline, cmdBuf);
            }

            if (subPass < subPassCount - 1) {
                cmdBuf.nextSubpass(vk::SubpassContents::eInline);
            }
        }

        renderPass.end(cmdBuf);
    }
    cmdBuf.end();

    return { cmdBuf };
}
