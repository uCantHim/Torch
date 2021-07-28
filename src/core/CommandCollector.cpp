#include "CommandCollector.h"

#include "TorchResources.h"



trc::CommandCollector::CommandCollector()
    :
    pool(vkb::getDevice()->createCommandPoolUnique(
        vk::CommandPoolCreateInfo(
            vk::CommandPoolCreateFlagBits::eResetCommandBuffer
            | vk::CommandPoolCreateFlagBits::eTransient,
            Queues::getMainRenderFamily()
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
    std::vector<vk::Viewport> viewports,
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
            for (auto pipeline : scene.getPipelines(stageType, SubPass::ID(subPass)))
            {
                // Bind the current pipeline
                auto& p = Pipeline::at(pipeline);
                p.bind(cmdBuf);
                p.bindStaticDescriptorSets(cmdBuf);
                p.bindDefaultPushConstantValues(cmdBuf);

                for (const auto& viewport : viewports)
                {
                    vk::Rect2D scissor({ 0, 0 }, { ui32(viewport.width), ui32(viewport.height) });
                    cmdBuf.setViewport(0, viewport);
                    cmdBuf.setScissor(0, scissor);

                    // Record commands for all objects with this pipeline
                    scene.invokeDrawFunctions(
                        stageType, renderPassId, SubPass::ID(subPass), pipeline,
                        cmdBuf
                    );
                }
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
