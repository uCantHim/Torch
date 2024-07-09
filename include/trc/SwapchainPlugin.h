#pragma once

#include "trc/core/RenderPlugin.h"

namespace trc
{
    class Swapchain;

    class SwapchainPlugin : public RenderPlugin
    {
    public:
        explicit SwapchainPlugin(const Swapchain& swapchain);

        void defineRenderStages(RenderGraph& renderGraph) override;
        void defineResources(ResourceConfig&) override {}

        auto createGlobalResources(RenderPipelineContext& ctx) -> u_ptr<GlobalResources> override;

    private:
        struct Instance : public GlobalResources
        {
            explicit Instance(const Swapchain& sc)
                : swapchain(sc) {}

            void registerResources(ResourceStorage&) override {}
            void hostUpdate(RenderPipelineContext&) override {}
            void createTasks(GlobalUpdateTaskQueue& taskQueue) override;

            const Swapchain& swapchain;
        };

        const Swapchain& swapchain;
    };
} // namespace trc
