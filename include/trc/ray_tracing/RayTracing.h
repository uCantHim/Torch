#pragma once

#include "core/RenderStage.h"

#include "AccelerationStructure.h"
#include "GeometryUtils.h"
#include "RayPipelineBuilder.h"
#include "ShaderBindingTable.h"
#include "RayBuffer.h"
#include "RayTracingPass.h"
#include "FinalCompositingPass.h"

namespace trc::rt
{
    inline RenderStage rayTracingRenderStage{};
} // namespace trc::rt
