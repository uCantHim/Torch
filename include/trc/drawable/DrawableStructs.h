#pragma once

#include "assets/Geometry.h"
#include "assets/Material.h"

namespace trc
{
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
