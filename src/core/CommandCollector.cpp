#include "CommandCollector.h"

#include "Window.h"
#include "TorchResources.h"
#include "utils/Util.h"



trc::CommandCollector::CommandCollector(
    const Window& window,
    vkb::QueueFamilyIndex renderQueueFamily)
    :
    device(window.getDevice()),
    swapchain(window.getSwapchain()),
    pool(window.getDevice()->createCommandPoolUnique(
        vk::CommandPoolCreateInfo(
            vk::CommandPoolCreateFlagBits::eResetCommandBuffer
            | vk::CommandPoolCreateFlagBits::eTransient,
            renderQueueFamily
        )
    ))
{
}

auto trc::CommandCollector::recordScene(
    const DrawConfig& draw,
    const RenderGraph& renderGraph
    ) -> std::vector<vk::CommandBuffer>
{
    // Ensure that enough command buffers are available
    while (renderGraph.size() > commandBuffers.size())
    {
        commandBuffers.emplace_back(
            swapchain,
            device->allocateCommandBuffersUnique(
                { *pool, vk::CommandBufferLevel::ePrimary, swapchain.getFrameCount() }
            )
        );
    }

    std::vector<vk::CommandBuffer> result;

    ui32 cmdBufIndex{ 0 };
    renderGraph.foreachStage(
        [&, this](RenderStageType::ID stage, const std::vector<RenderPass*>& passes)
        {
            auto cmdBuf = **commandBuffers.at(cmdBufIndex++);
            recordStage(cmdBuf, draw, stage, passes);

            result.emplace_back(cmdBuf);
        }
    );

    return result;
}

void trc::CommandCollector::recordStage(
    vk::CommandBuffer cmdBuf,
    const DrawConfig& draw,
    RenderStageType::ID stage,
    const std::vector<RenderPass*>& passes)
{
    SceneBase& scene = *draw.scene;
    const auto& [viewport, scissor] = draw.renderArea;

    cmdBuf.reset({});
    cmdBuf.begin({ vk::CommandBufferUsageFlagBits::eOneTimeSubmit });

    for (auto renderPass : util::merged(passes, scene.getDynamicRenderPasses(stage)))
    {
        assert(renderPass != nullptr);

        renderPass->begin(cmdBuf, vk::SubpassContents::eInline);

        // Record all commands
        const ui32 subPassCount = renderPass->getNumSubPasses();
        for (ui32 subPass = 0; subPass < subPassCount; subPass++)
        {
            for (auto pipeline : scene.getPipelines(stage, SubPass::ID(subPass)))
            {
                // Bind the current pipeline
                auto& p = draw.renderConfig->getPipeline(pipeline);
                p.bind(cmdBuf);
                p.bindStaticDescriptorSets(cmdBuf);
                p.bindDefaultPushConstantValues(cmdBuf);

                cmdBuf.setViewport(0, viewport);
                cmdBuf.setScissor(0, scissor);

                // Record commands for all objects with this pipeline
                scene.invokeDrawFunctions(
                    stage, *renderPass, SubPass::ID(subPass),
                    pipeline, p,
                    cmdBuf
                );
            }

            if (subPass < subPassCount - 1) {
                cmdBuf.nextSubpass(vk::SubpassContents::eInline);
            }
        }

        renderPass->end(cmdBuf);
    }
    cmdBuf.end();
}
