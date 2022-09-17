#pragma once

#include "trc/Types.h"
#include "Vec2D.h"
#include "FormattedValue.h"

namespace trc::ui
{
    struct Transform
    {
        vec2 position{ 0.0f, 0.0f };
        vec2 size{ 1.0f, 1.0f };

        // TODO: Position should be in pixels by default
        Vec2D<Format> positionFormat{ Format::eNorm };
        Vec2D<Format> sizeFormat{ Format::ePixel };
        Vec2D<Scale> scalingType{ Scale::eAbsolute };
    };

    class Window;

    auto concat(Transform parent, Transform child, const Window& window) noexcept -> Transform;
} // namespace trc::ui
