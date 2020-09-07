#pragma once

#include <vkb/basics/Swapchain.h>

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
        eDeferredPass = 0,

        /** The index of the first shadow pass */
        eShadowPassesBegin = 1,
        eShadowPassesEnd = MAX_SHADOW_MAPS,

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
    };

    void makeAllDrawablePipelines();

    // Deferred pipelines
    void makeDrawableDeferredPipeline(RenderPass& renderPass);
    void makeDrawableDeferredAnimatedPipeline(RenderPass& renderPass);
    void makeDrawableDeferredPickablePipeline(RenderPass& renderPass);
    void makeDrawableDeferredAnimatedAndPickablePipeline(RenderPass& renderPass);
    void _makeDrawableDeferredPipeline(ui32 pipelineIndex, ui32 featureFlags, RenderPass& rp);
    void makeDrawableTransparentPipeline(ui32 pipelineIndex, ui32 featureFlags, RenderPass& rp);
    void makeInstancedDrawableDeferredPipeline(RenderPass& renderPass);

    // Shadow pipelines
    void makeDrawableShadowPipeline(RenderPassShadow& renderPass);
    void makeInstancedDrawableShadowPipeline(RenderPassShadow& renderPass);

    // Final lighting pipeline
    void makeFinalLightingPipeline(
        RenderPass& renderPass,
        const DescriptorProviderInterface& gBufferInputSet);
}
