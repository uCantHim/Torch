#pragma once

#include "core/RenderStage.h"

namespace trc
{
    inline RenderStage deferredRenderStage{};
    inline RenderStage finalLightingRenderStage{};
    inline RenderStage shadowRenderStage{};
} // namespace trc
