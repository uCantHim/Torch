#pragma once

#include "Transformation.h"
#include "AnimationEngine.h"
#include "AssetIds.h"

namespace trc
{
    struct DrawableInstanceCreateInfo
    {
        Transformation::ID transform;
        AnimationEngine::ID animData;
    };

    struct DrawableCreateInfo
    {
        GeometryID geo;
        MaterialID mat;

        bool transparent{ false };
        bool drawShadow{ true };

        bool rasterized{ true };
        bool rayTraced{ true };
    };
} // namespace trc
