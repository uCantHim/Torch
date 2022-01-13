#pragma once

#include "core/RenderStage.h"

namespace trc
{
    inline RenderStage gBufferRenderStage{};
    inline RenderStage finalLightingRenderStage{};
    inline RenderStage shadowRenderStage{};
} // namespace trc
