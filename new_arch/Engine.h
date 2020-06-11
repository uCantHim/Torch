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
        static const std::vector<vk::ClearValue> clearValues = {
            vk::ClearColorValue(std::array<float, 4>{ 1.0, 0.4, 1.0 }),
            vk::ClearDepthStencilValue(1.0f, 0),
        };

        auto& swapchain = vkb::VulkanBase::getSwapchain();

        // Set up rendering
        cmdBuf->reset({});

        cmdBuf->begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));
        cmdBuf->beginRenderPass(
            vk::RenderPassBeginInfo(
                *renderPass,
                renderPass.getFramebuffer(),
                vk::Rect2D({ 0, 0 }, swapchain.getImageExtent()),
                static_cast<uint32_t>(clearValues.size()), clearValues.data()
            ),
            vk::SubpassContents::eInline
        );

        // Record all commands
        const auto& subPasses = renderPass.getSubPasses();
        for (size_t i = 0; i < subPasses.size(); i++)
        {
            auto& subPass = subPasses[i];

            std::cout << "Subpass " << subPass << " started\n";
            for (auto pipeline : scene.getPipelines(subPass))
            {
                std::cout << "Pipeline " << pipeline << " started\n";
                // Bind the current pipeline
                GraphicsPipeline::at(pipeline).bind(*cmdBuf, false);

                // Record commands for all objects with this pipeline
                scene.invokeDrawFunctions(subPass, pipeline, *cmdBuf);
            }

            if (i < subPasses.size() - 1) {
                cmdBuf->nextSubpass(vk::SubpassContents::eInline);
            }
        }

        cmdBuf->endRenderPass();
        cmdBuf->end();
        std::cout << "Recording finished\n";

        // Submit work and present the image
        auto semaphore = vkb::VulkanBase::getDevice()->createSemaphoreUnique(
            vk::SemaphoreCreateInfo(vk::SemaphoreCreateFlags())
        );
        auto image = swapchain.acquireImage(*semaphore);

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
