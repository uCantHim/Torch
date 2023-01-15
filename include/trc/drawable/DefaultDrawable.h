#pragma once

#include "trc/DrawablePipelines.h"
#include "trc/drawable/DrawableStructs.h"
#include "trc/drawable/DrawableComponentScene.h"

namespace trc
{
    struct DrawablePipelineInfo
    {
        bool animated;
        bool transparent;
    };

    auto getDrawablePipelineFlags(DrawablePipelineInfo info) -> pipelines::DrawablePipelineTypeFlags;

    auto determineDrawablePipeline(DrawablePipelineInfo info) -> Pipeline::ID;
    auto determineDrawablePipeline(const DrawableCreateInfo& info) -> Pipeline::ID;

    auto makeDefaultDrawableRasterization(const DrawableCreateInfo& info)
        -> RasterComponentCreateInfo;
    auto makeDefaultDrawableRasterization(const DrawableCreateInfo& info, Pipeline::ID pipeline)
        -> RasterComponentCreateInfo;
} // namespace trc
