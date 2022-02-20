#pragma once

#include "DrawablePoolStructs.h"
#include "DrawableComponentScene.h"

namespace trc
{
    auto makeDrawableRasterization(const DrawableCreateInfo& info)
        -> RasterComponentCreateInfo;
    auto makeDefaultDrawableRasterization(const DrawableCreateInfo& info, Pipeline::ID pipeline)
        -> RasterComponentCreateInfo;
} // namespace trc
