#pragma once

#include "Types.h"
#include "core/Pipeline.h"
#include "FlagCombination.h"

namespace trc
{
    enum class PipelineShadingTypeFlagBits
    {
        eOpaque      = 0,
        eTransparent = 1,
        eShadow      = 2,

        eMaxEnum     = 3,
    };

    enum class PipelineVertexTypeFlagBits
    {
        eMesh     = 0,
        eSkeletal = 1,

        eMaxEnum  = 2,
    };

    enum class PipelineAnimationTypeFlagBits
    {
        eNone     = 0,
        eAnimated = 1,

        eMaxEnum  = 2,
    };

    using PipelineFlags = FlagCombination<
        PipelineShadingTypeFlagBits,
        PipelineVertexTypeFlagBits,
        PipelineAnimationTypeFlagBits
    >;

    auto getPipeline(PipelineFlags flags) -> Pipeline::ID;
} // namespace trc
