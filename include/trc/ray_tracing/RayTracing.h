#pragma once

#include "core/RenderStage.h"

#include "AccelerationStructure.h"
#include "GeometryUtils.h"
#include "RayPipelineBuilder.h"
#include "ShaderBindingTable.h"
#include "RayTracingPass.h"
#include "RayBuffer.h"

namespace trc::rt
{
    inline auto getRayTracingRenderStageType() -> RenderStageType::ID
    {
        static auto id = RenderStageType::createAtNextIndex(1).first;
        return id;
    }
} // namespace trc::rt
