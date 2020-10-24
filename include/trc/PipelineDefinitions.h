#pragma once

#include <vkb/basics/Swapchain.h>

#include "Renderer.h"
#include "RenderStage.h"
#include "RenderPass.h"
#include "RenderPassShadow.h"
#include "Pipeline.h"

namespace trc::internal
{
    enum RenderStages : RenderStage::ID::Type
    {
        eDeferred,
        eShadow,

        NUM_STAGES
    };

    enum RenderPasses : RenderPass::ID::Type
    {
        // Unused, index is allocated per renderer
        //eDeferredPass = 0,

        /** The index of the first shadow pass */
        eShadowPassesBegin = 1,
        eShadowPassesEnd = _ShadowDescriptor::MAX_SHADOW_MAPS,

        NUM_PASSES
    };

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
