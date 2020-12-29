#pragma once

#include "../RenderStage.h"
#include "AccelerationStructure.h"

namespace trc::rt
{
    inline auto getRayTracingRenderStageType() -> RenderStageType::ID
    {
        static auto id = RenderStageType::createAtNextIndex(1).first;
        return id;
    }
} // namespace trc::rt
