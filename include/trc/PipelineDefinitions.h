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

        NUM_PIPELINES
    };

    void makeAllDrawablePipelines(const Renderer& renderer);

    // Deferred pipelines
    void makeDrawableDeferredPipeline(const Renderer& renderer);
    void makeDrawableDeferredAnimatedPipeline(const Renderer& renderer);
    void makeDrawableDeferredPickablePipeline(const Renderer& renderer);
    void makeDrawableDeferredAnimatedAndPickablePipeline(const Renderer& renderer);

    void _makeDrawableDeferredPipeline(ui32 pipelineIndex, ui32 featureFlags, const Renderer& rp);
    void makeDrawableTransparentPipeline(ui32 pipelineIndex, ui32 featureFlags, const Renderer& rp);

    void makeInstancedDrawableDeferredPipeline(const Renderer& renderer);

    // Shadow pipelines
    void makeDrawableShadowPipeline(const Renderer& renderer, RenderPassShadow& renderPass);
    void makeInstancedDrawableShadowPipeline(const Renderer& renderer, RenderPassShadow& renderPass);

    // Final lighting pipeline
    void makeFinalLightingPipeline(const Renderer& renderer);

} // namespace trc::internal
