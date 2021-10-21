#pragma once

#include "RenderStage.h"

namespace trc
{
    /**
     * @brief Access to global render stage types
     */
    struct RenderStageTypes
    {
        static auto getDeferred() -> RenderStageType::ID;
        static auto getShadow() -> RenderStageType::ID;
    };
} // namespace trc
