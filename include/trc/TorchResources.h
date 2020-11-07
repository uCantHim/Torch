#pragma once

#include <vkb/basics/Instance.h>
#include <vkb/basics/Device.h>

#include "RenderStage.h"

namespace trc
{
    enum RenderStageTypes : RenderStageType::ID::Type
    {
        eDeferred,
        eShadow,

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
