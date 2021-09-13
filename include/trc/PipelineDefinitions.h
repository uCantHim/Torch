#pragma once

#include <vkb/basics/Swapchain.h>

#include "Pipeline.h"

/**
 * @brief Define a getter function for a pipeline type
 *
 * Saves some standard boilerplate for defining a getter function for a
 * pipeline ID, which is a type of pipeline.
 */
#define PIPELINE_GETTER_FUNC(Name, Factory, RenderConfigType)                           \
    auto Name() -> Pipeline::ID                                                         \
    {                                                                                   \
        static auto id = PipelineRegistry<RenderConfigType>::registerPipeline(Factory); \
        return id;                                                                      \
    }

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
