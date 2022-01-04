#pragma once

#include "core/Pipeline.h"
#include "core/PipelineRegistry.h"

/**
 * @brief Define a getter function for a pipeline type
 *
 * Saves some standard boilerplate for defining a getter function for a
 * pipeline ID, which is a type of pipeline.
 */
#define PIPELINE_GETTER_FUNC(_Name, _RegistrationFunc)      \
    auto _Name() -> Pipeline::ID                            \
    {                                                       \
        static auto id = _RegistrationFunc();               \
        return id;                                          \
    }

namespace trc
{
    static const fs::path SHADER_DIR{ TRC_SHADER_DIR };
} // namespace trc::internal
