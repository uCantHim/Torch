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
        //eDeferredPass = 0,

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

        NUM_PIPELINES
    };

    void makeAllDrawablePipelines(const Renderer& renderer);

    // Deferred pipelines
    void makeDrawableDeferredPipeline(const RenderPassDeferred& renderPass);
    void makeDrawableDeferredAnimatedPipeline(const RenderPassDeferred& renderPass);
    void makeDrawableDeferredPickablePipeline(const RenderPassDeferred& renderPass);
    void makeDrawableDeferredAnimatedAndPickablePipeline(const RenderPassDeferred& renderPass);

    void _makeDrawableDeferredPipeline(ui32 pipelineIndex,
                                       ui32 featureFlags,
                                       const RenderPassDeferred& rp);
    void makeDrawableTransparentPipeline(ui32 pipelineIndex,
                                         ui32 featureFlags,
                                         const RenderPassDeferred& rp);

    void makeInstancedDrawableDeferredPipeline(const RenderPassDeferred& renderPass);

    // Shadow pipelines
    void makeDrawableShadowPipeline(RenderPassShadow& renderPass);
    void makeInstancedDrawableShadowPipeline(RenderPassShadow& renderPass);

    // Final lighting pipeline
    void makeFinalLightingPipeline(const Renderer& renderer);

} // namespace trc::internal
