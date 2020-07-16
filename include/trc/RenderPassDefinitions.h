#pragma once

#include <vkb/basics/Swapchain.h>

#include "Renderpass.h"
#include "Pipeline.h"

namespace trc::internal
{
    enum RenderPasses : SubPass::ID
    {
        eDeferredPass = 0,
        eLightingPass = 1,

        NUM_PASSES
    };


    enum Pipelines : GraphicsPipeline::ID
    {
        eDrawableDeferred = 0,
        eDrawableLighting = 1,
    };


    void initRenderEnvironment();

    void makeMainRenderPass();

    void makeDrawableDeferredPipeline();

    struct RenderEnvRecreateHelper : public vkb::SwapchainDependentResource
    {
        void signalRecreateRequired() override;
        void recreate(vkb::Swapchain& swapchain) override;
        void signalRecreateFinished() override;
    };

    static RenderEnvRecreateHelper envRecreateHelper;
}
