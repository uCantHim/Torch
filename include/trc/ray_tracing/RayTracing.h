#pragma once

#include "RenderStage.h"
#include "AccelerationStructure.h"
#include "RayPipelineBuilder.h"
#include "ShaderBindingTable.h"

namespace trc::rt
{
    inline auto getRayTracingRenderStageType() -> RenderStageType::ID
    {
        static auto id = RenderStageType::createAtNextIndex(1).first;
        return id;
    }
} // namespace trc::rt
