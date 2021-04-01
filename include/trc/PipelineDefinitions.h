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

    //enum Pipelines : Pipeline::ID::Type
    //{
    //    eDrawableDeferred = 0,
    //    eDrawableDeferredAnimated = 1,
    //    eDrawableDeferredPickable = 2,
    //    eDrawableDeferredAnimatedAndPickable = 3,
    //    eDrawableTransparentDeferred,
    //    eDrawableTransparentDeferredAnimated,
    //    eDrawableTransparentDeferredPickable,
    //    eDrawableTransparentDeferredAnimatedAndPickable,
    //    eFinalLighting,
    //    eDrawableInstancedDeferred,

    //    eDrawableShadow,
    //    eDrawableInstancedShadow,

    //    eParticleDraw,
    //    eParticleShadow,

    //    eText,

    //    NUM_PIPELINES
    //};

    static const fs::path SHADER_DIR{ TRC_SHADER_DIR };

    auto getDrawableDeferredPipeline() -> Pipeline::ID;
    auto getDrawableDeferredAnimatedPipeline() -> Pipeline::ID;
    auto getDrawableDeferredPickablePipeline() -> Pipeline::ID;
    auto getDrawableDeferredAnimatedAndPickablePipeline() -> Pipeline::ID;

    auto getDrawableTransparentDeferredPipeline() -> Pipeline::ID;
    auto getDrawableTransparentDeferredAnimatedPipeline() -> Pipeline::ID;
    auto getDrawableTransparentDeferredPickablePipeline() -> Pipeline::ID;
    auto getDrawableTransparentDeferredAnimatedAndPickablePipeline() -> Pipeline::ID;

    auto getDrawableShadowPipeline() -> Pipeline::ID;

    auto getDrawableInstancedDeferredPipeline() -> Pipeline::ID;
    auto getDrawableInstancedShadowPipeline() -> Pipeline::ID;

    // Final lighting pipeline
    auto getFinalLightingPipeline() -> Pipeline::ID;
} // namespace trc::internal
