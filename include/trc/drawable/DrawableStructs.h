#pragma once

#include "trc/assets/Geometry.h"
#include "trc/assets/Material.h"
#include "trc/AnimationEngine.h"

namespace trc
{
    struct DrawableCreateInfo
    {
        GeometryID geo;
        MaterialID mat;

        bool rasterized{ true };
        bool rayTraced{ false };
    };

    struct DrawablePushConstants
    {
        mat4 model{ 1.0f };
        ui32 materialIndex{ 0u };

        ui32 animationIndex{ NO_ANIMATION };
        uvec2 keyframes{ 0, 0 };
        float keyframeWeight{ 0.0f };
    };
} // namespace trc
