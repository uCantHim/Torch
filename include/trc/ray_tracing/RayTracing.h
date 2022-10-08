#pragma once

#include "trc/core/RenderStage.h"

#include "trc/ray_tracing/AccelerationStructure.h"
#include "trc/ray_tracing/FinalCompositingPass.h"
#include "trc/ray_tracing/GeometryUtils.h"
#include "trc/ray_tracing/RayBuffer.h"
#include "trc/ray_tracing/RayPipelineBuilder.h"
#include "trc/ray_tracing/RayTracingPass.h"
#include "trc/ray_tracing/ShaderBindingTable.h"

namespace trc::rt
{
    inline RenderStage rayTracingRenderStage{};
} // namespace trc::rt
