#pragma once

#include "Types.h"
#include "core/Pipeline.h"
#include "trc/DrawablePipelines.h"  // Generated drawable pipeline definitions

namespace trc
{
    auto getPipeline(pipelines::DrawablePipelineTypeFlags flags) -> Pipeline::ID;
} // namespace trc
