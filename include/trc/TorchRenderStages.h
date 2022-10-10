#pragma once

#include "trc/core/RenderStage.h"

namespace trc
{
    inline RenderStage resourceUpdateStage{};

    inline RenderStage gBufferRenderStage{};
    inline RenderStage shadowRenderStage{};
    inline RenderStage finalLightingRenderStage{};

    inline RenderStage rayTracingRenderStage{};
} // namespace trc
