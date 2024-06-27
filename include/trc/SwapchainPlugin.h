#pragma once

#include "trc/core/RenderPlugin.h"

namespace trc
{
    class SwapchainPlugin : public RenderPlugin
    {
    public:
        void registerRenderStages(RenderGraph& renderGraph) override;
        void defineResources(ResourceConfig&) override {}

        auto createDrawConfig(const Device& device, Viewport renderTarget)
            -> u_ptr<DrawConfig> override;

    private:
        struct SwapchainPluginDrawConfig : public DrawConfig
        {
            explicit SwapchainPluginDrawConfig(vk::Image swapchainImage)
                : image(swapchainImage) {}

            void registerResources(ResourceStorage&) override {}
            void update(const Device&, SceneBase&, const Camera&) override {}

            void createTasks(SceneBase& scene, TaskQueue& taskQueue) override;

            vk::Image image;
        };
    };
} // namespace trc
