#pragma once

#include "RenderStage.h"

namespace trc
{
    enum RenderStageTypes : RenderStageType::ID::Type
    {
        eDeferred,
        eShadow,
        ePostProcessing,

        NUM_STAGES
    };

    class DeferredStage : public RenderStage
    {
    public:
        DeferredStage() : RenderStage(RenderStageType::at(RenderStageTypes::eDeferred)) {}
    };

    /**
     * @brief A render stage that renders shadow maps
     */
    class ShadowStage : public RenderStage
    {
    public:
        ShadowStage() : RenderStage(RenderStageType::at(RenderStageTypes::eShadow)) {}
    };
} // namespace trc
