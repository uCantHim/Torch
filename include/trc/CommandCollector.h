#pragma once

#include <vkb/VulkanBase.h>

#include "Boilerplate.h"
#include "RenderPass.h"
#include "base/SceneBase.h"
#include "utils/Camera.h"

namespace trc
{
    struct DrawInfo
    {
        RenderPass* renderPass;
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

            const auto& [renderPass, camera] = drawInfo;
            auto cmdBuf = **commandBuffers;

            // Set up rendering
            cmdBuf.reset({});
            cmdBuf.begin({ vk::CommandBufferUsageFlagBits::eOneTimeSubmit });
            renderPass->begin(cmdBuf, vk::SubpassContents::eInline);

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

            renderPass->end(cmdBuf);
            cmdBuf.end();

            return cmdBuf;
        }

    private:
        vkb::FrameSpecificObject<vk::UniqueCommandBuffer> commandBuffers;
    };
} // namespace trc
