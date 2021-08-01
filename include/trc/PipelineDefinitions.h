#pragma once

#include <vkb/basics/Swapchain.h>

#include "Pipeline.h"

namespace trc::internal
{
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
