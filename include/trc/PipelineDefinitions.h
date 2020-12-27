#pragma once

#include <vkb/basics/Swapchain.h>

#include "Pipeline.h"
#include "LightRegistry.h"
#include "RenderPassShadow.h"

namespace trc {
    class Renderer;
}

namespace trc::internal
{
    enum DeferredSubPasses
    {
        eGBufferPass = 0,
        eTransparencyPass = 1,
        eLightingPass = 2,

        NUM_SUBPASSES
    };

    enum Pipelines : Pipeline::ID::Type
    {
        eDrawableDeferred = 0,
        eDrawableDeferredAnimated = 1,
        eDrawableDeferredPickable = 2,
        eDrawableDeferredAnimatedAndPickable = 3,
        eDrawableTransparentDeferred,
        eDrawableTransparentDeferredAnimated,
        eDrawableTransparentDeferredPickable,
        eDrawableTransparentDeferredAnimatedAndPickable,
        eFinalLighting,
        eDrawableInstancedDeferred,

        eDrawableShadow,
        eDrawableInstancedShadow,

        eParticleDraw,
        eParticleShadow,

        eText,

        NUM_PIPELINES
    };

    void makeAllDrawablePipelines(vk::RenderPass deferredPass);

    // Deferred pipelines
    void makeDrawableDeferredPipeline(vk::RenderPass deferredPass);
    void makeDrawableDeferredAnimatedPipeline(vk::RenderPass deferredPass);
    void makeDrawableDeferredPickablePipeline(vk::RenderPass deferredPass);
    void makeDrawableDeferredAnimatedAndPickablePipeline(vk::RenderPass deferredPass);

    void _makeDrawableDeferredPipeline(ui32 pipelineIndex,
                                       ui32 featureFlags,
                                       vk::RenderPass deferredPass);
    void makeDrawableTransparentPipeline(ui32 pipelineIndex,
                                         ui32 featureFlags,
                                         vk::RenderPass deferredPass);

    void makeInstancedDrawableDeferredPipeline(vk::RenderPass deferredPass);

    // Shadow pipelines
    void makeDrawableShadowPipeline(RenderPassShadow& renderPass);
    void makeInstancedDrawableShadowPipeline(RenderPassShadow& renderPass);

    // Final lighting pipeline
    void makeFinalLightingPipeline(vk::RenderPass deferredPass);

} // namespace trc::internal
