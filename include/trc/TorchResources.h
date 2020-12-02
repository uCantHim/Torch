#pragma once

#include <vkb/basics/Instance.h>
#include <vkb/basics/Device.h>

#include "RenderStage.h"

namespace trc
{
    struct RenderStageTypes
    {
        static auto getDeferred() -> RenderStageType::ID
        {
            static auto deferred = RenderStageType::createAtNextIndex(3).first;
            return deferred;
        }

        static auto getShadow() -> RenderStageType::ID
        {
            static auto shadow = RenderStageType::createAtNextIndex(1).first;
            return shadow;
        }
    };
} // namespace trc
