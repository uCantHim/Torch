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
    inline auto getRayTracingRenderStage() -> RenderStageType::ID
    {
        static auto id = RenderStageType::createAtNextIndex(1).first;
        return id;
    }

    inline auto getFinalCompositingStage() -> RenderStageType::ID
    {
        static auto id = RenderStageType::createAtNextIndex(FinalCompositingPass::NUM_SUBPASSES).first;
        return id;
    }
} // namespace trc::rt
