#include "CommandCollector.h"

#include "Window.h"

#include "TorchResources.h"



trc::CommandCollector::CommandCollector(const Instance& instance, const Window& window)
    :
    pool(instance.getDevice()->createCommandPoolUnique(
        vk::CommandPoolCreateInfo(
            vk::CommandPoolCreateFlagBits::eResetCommandBuffer
            | vk::CommandPoolCreateFlagBits::eTransient,
            Queues::getMainRenderFamily()
        )
    )),
    commandBuffers(
        window.getSwapchain(),
        instance.getDevice()->allocateCommandBuffersUnique(
            { *pool, vk::CommandBufferLevel::ePrimary, window.getSwapchain().getFrameCount() }
        )
    )
{
}

auto trc::CommandCollector::recordScene(
    const DrawConfig& draw,
    RenderStageType::ID stageType,
    const std::vector<RenderPass*>& passes
    ) -> vk::CommandBuffer
{
    SceneBase& scene = *draw.scene;
    auto cmdBuf = **commandBuffers;

    // Set up rendering
    cmdBuf.reset({});
    cmdBuf.begin({ vk::CommandBufferUsageFlagBits::eOneTimeSubmit });
    for (auto renderPass : passes)
    {
        assert(renderPass != nullptr);

        renderPass->begin(cmdBuf, vk::SubpassContents::eInline);

        // Record all commands
        const ui32 subPassCount = renderPass->getNumSubPasses();
        for (ui32 subPass = 0; subPass < subPassCount; subPass++)
        {
            for (auto pipeline : scene.getPipelines(stageType, SubPass::ID(subPass)))
            {
                // Bind the current pipeline
                auto& p = draw.renderConfig->getPipeline(pipeline);
                p.bind(cmdBuf);
                p.bindStaticDescriptorSets(cmdBuf);
                p.bindDefaultPushConstantValues(cmdBuf);

                for (const auto& [viewport, scissor] : draw.renderAreas)
                {
                    cmdBuf.setViewport(0, viewport);
                    cmdBuf.setScissor(0, scissor);

                    // Record commands for all objects with this pipeline
                    scene.invokeDrawFunctions(
                        stageType, *renderPass, SubPass::ID(subPass),
                        pipeline, p,
                        cmdBuf
                    );
                }
            }

            if (subPass < subPassCount - 1) {
                cmdBuf.nextSubpass(vk::SubpassContents::eInline);
            }
        }

        renderPass->end(cmdBuf);
    }
    cmdBuf.end();

    return { cmdBuf };
}
