#pragma once

#include "Types.h"
#include "core/Pipeline.h"
#include "trc/Pipelines.h"  // Generated drawable pipeline definitions

namespace trc
{
    auto getPipeline(DrawablePipelineTypeFlags flags) -> Pipeline::ID;
} // namespace trc
