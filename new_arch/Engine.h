#pragma once

#include <vkb/VulkanBase.h>

#include "Renderpass.h"
#include "Scene.h"

class Engine
{
public:
    Engine()
        : cmdBuf(vkb::VulkanBase::getDevice().createGraphicsCommandBuffer())
    {}

    /**
     * Do some cool things here
     */
    void drawScene(RenderPass& renderPass, Scene& scene)
    {
        static const vk::ClearValue clearColor(vk::ClearColorValue(std::array<float, 4>{ 1.0, 0.4, 1.0 }));

        auto& swapchain = vkb::VulkanBase::getSwapchain();
        const auto swapchainSize = swapchain.getImageExtent();

        // Set up rendering
        cmdBuf->reset({});

        cmdBuf->begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));
        cmdBuf->beginRenderPass(
            vk::RenderPassBeginInfo(
                *renderPass,
                renderPass.getFramebuffer(),
                vk::Rect2D{ swapchainSize.width, swapchainSize.height },
                1u, &clearColor
            ),
            vk::SubpassContents::eInline
        );

        for (auto subPass : renderPass.getSubPasses())
        {
            for (auto pipeline : scene.getPipelines(subPass))
            {
                // Bind the current pipeline
                GraphicsPipeline::at(pipeline).bind(*cmdBuf, true);

                // Record commands for all objects with this pipeline
                scene.invokeDrawFunctions(subPass, pipeline, *cmdBuf);
            }

            cmdBuf->nextSubpass(vk::SubpassContents::eInline);
        }

        cmdBuf->endRenderPass();
        cmdBuf->end();


        auto image = swapchain.acquireImage({});

        vkb::VulkanBase::getDevice().executeGraphicsCommandBufferSynchronously(*cmdBuf);

        swapchain.presentImage(
            image,
            vkb::VulkanBase::getQueueProvider().getQueue(vkb::queue_type::presentation),
            {}
        );
    }

private:
    vk::UniqueCommandBuffer cmdBuf;
};
