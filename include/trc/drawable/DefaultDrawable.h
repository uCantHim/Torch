#pragma once

#include "trc/drawable/DrawableStructs.h"
#include "trc/drawable/DrawableComponentScene.h"

namespace trc
{
    auto determineDrawablePipeline(const DrawableCreateInfo& info) -> Pipeline::ID;

    auto makeDefaultDrawableRasterization(const DrawableCreateInfo& info, Pipeline::ID pipeline)
        -> RasterComponentCreateInfo;
} // namespace trc
