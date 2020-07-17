#pragma once

#include <vkb/VulkanBase.h>

#include "Boilerplate.h"
#include "Renderpass.h"
#include "base/SceneBase.h"
#include "utils/Camera.h"

namespace trc
{
    struct DrawInfo
    {
        RenderPass* renderPass;
        vk::Framebuffer framebuffer;
        const Camera* camera;
    };

    class CommandCollector
    {
    public:
        CommandCollector()
            :
            commandBuffers(
                [](ui32) {
                    return vkb::VulkanBase::getDevice().createGraphicsCommandBuffer();
                }
            )
        {}

        /**
         * @brief Draw a scene to a frame
         *
         * TODO: This is a trivial implementation. Do some fancy threaded stuff here.
         */
        auto recordScene(SceneBase& scene, const DrawInfo& drawInfo) -> vk::CommandBuffer
        {
            assert(drawInfo.renderPass != nullptr);

            const auto& clearValues = drawInfo.renderPass->getClearValues();
            const auto& [renderPass, framebuffer, camera] = drawInfo;
            Viewport viewport = camera->getViewport();

            auto cmdBuf = **commandBuffers;

            // Set up rendering
            cmdBuf.reset({});

            cmdBuf.begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));
            cmdBuf.beginRenderPass(
                vk::RenderPassBeginInfo(
                    **renderPass,
                    framebuffer,
                    vk::Rect2D(
                        { viewport.offset.x, viewport.offset.y },
                        { viewport.size.x, viewport.size.y }
                    ),
                    static_cast<ui32>(clearValues.size()), clearValues.data()
                ),
                vk::SubpassContents::eInline
            );

            // Record all commands
            const ui32 subPassCount = drawInfo.renderPass->getNumSubPasses();
            for (ui32 subPass = 0; subPass < subPassCount; subPass++)
            {
                for (auto pipeline : scene.getPipelines(subPass))
                {
                    // Bind the current pipeline
                    auto& p = GraphicsPipeline::at(pipeline);
                    p.bind(cmdBuf);
                    p.bindStaticDescriptorSets(cmdBuf);

                    // Record commands for all objects with this pipeline
                    scene.invokeDrawFunctions(subPass, pipeline, cmdBuf);
                }

                if (subPass < subPassCount - 1) {
                    cmdBuf.nextSubpass(vk::SubpassContents::eInline);
                }
            }

            cmdBuf.endRenderPass();
            cmdBuf.end();

            return cmdBuf;
        }

    private:
        vkb::FrameSpecificObject<vk::UniqueCommandBuffer> commandBuffers;
    };
} // namespace trc
