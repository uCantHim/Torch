#pragma once

#include <vkb/VulkanBase.h>
#include <glm/glm.hpp>
using namespace glm;

#include "Renderpass.h"
#include "base/SceneBase.h"

struct Viewport
{
    ivec2 offset;
    uvec2 size;
};

struct DrawInfo
{
    RenderPass* renderPass;
    vk::Framebuffer framebuffer;
    Viewport viewport;
};

class CommandCollector
{
public:
    CommandCollector()
        : cmdBuf(vkb::VulkanBase::getDevice().createGraphicsCommandBuffer())
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
        const auto& [renderPass, framebuffer, viewport] = drawInfo;

        // Set up rendering
        cmdBuf->reset({});

        cmdBuf->begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));
        cmdBuf->beginRenderPass(
            vk::RenderPassBeginInfo(
                **renderPass,
                framebuffer,
                vk::Rect2D(
                    { viewport.offset.x, viewport.offset.y },
                    { viewport.size.x, viewport.size.y }
                ),
                static_cast<uint32_t>(clearValues.size()), clearValues.data()
            ),
            vk::SubpassContents::eInline
        );

        // Record all commands
        const uint32_t subPassCount = drawInfo.renderPass->getNumSubPasses();
        for (uint32_t subPass = 0; subPass < subPassCount; subPass++)
        {
            for (auto pipeline : scene.getPipelines(subPass))
            {
                // Bind the current pipeline
                GraphicsPipeline::at(pipeline).bind(*cmdBuf, false);

                // Record commands for all objects with this pipeline
                scene.invokeDrawFunctions(subPass, pipeline, *cmdBuf);
            }

            if (subPass < subPassCount - 1) {
                cmdBuf->nextSubpass(vk::SubpassContents::eInline);
            }
        }

        cmdBuf->endRenderPass();
        cmdBuf->end();

        return *cmdBuf;
    }

private:
    vk::UniqueCommandBuffer cmdBuf;
};
