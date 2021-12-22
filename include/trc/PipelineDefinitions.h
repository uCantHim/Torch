#pragma once

#include <vkb/Swapchain.h>

#include "Pipeline.h"
#include "PipelineRegistry.h"

/**
 * @brief Define a getter function for a pipeline type
 *
 * Saves some standard boilerplate for defining a getter function for a
 * pipeline ID, which is a type of pipeline.
 */
#define PIPELINE_GETTER_FUNC(_Name, _Factory, _RenderConfigType)                             \
    auto _Name() -> Pipeline::ID                                                             \
    {                                                                                        \
        static auto id = PipelineRegistry<_RenderConfigType>::registerPipeline(_Factory());  \
        return id;                                                                           \
    }

namespace trc
{
    static const fs::path SHADER_DIR{ TRC_SHADER_DIR };

    // Final lighting pipeline
    auto getFinalLightingPipeline() -> Pipeline::ID;
} // namespace trc::internal
