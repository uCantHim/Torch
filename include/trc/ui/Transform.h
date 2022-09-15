#pragma once

#include "trc/Types.h"
#include "Vec2D.h"
#include "FormattedValue.h"

namespace trc::ui
{
    struct Transform
    {
        struct Properties
        {
            Vec2D<Format> format;
            Vec2D<Align> align;
        };

        vec2 position{ 0.0f, 0.0f };
        vec2 size{ 1.0f, 1.0f };

        // TODO: Absolut positioning doesn't make any sense
        // TODO: Position should be in pixels by default
        Properties posProp{ Format::eNorm, Align::eRelative };
        Properties sizeProp{ Format::ePixel, Align::eAbsolute };
    };

    class Window;

    auto concat(Transform parent, Transform child, const Window& window) noexcept -> Transform;
} // namespace trc::ui
