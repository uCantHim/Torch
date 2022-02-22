#pragma once

#include "DrawableStructs.h"
#include "DrawableComponentScene.h"

namespace trc
{
    auto determineDrawablePipeline(const DrawableCreateInfo& info) -> Pipeline::ID;

    auto makeDefaultDrawableRasterization(const DrawableCreateInfo& info, Pipeline::ID pipeline)
        -> RasterComponentCreateInfo;
} // namespace trc
