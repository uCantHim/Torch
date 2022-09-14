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
            Vec2D<Format> format{ Format::eNorm };
            Vec2D<Align> align{ Align::eRelative };
        };

        vec2 position{ 0.0f, 0.0f };
        vec2 size{ 1.0f, 1.0f };

        Properties posProp{};
        Properties sizeProp{};
    };

    class Window;

    auto concat(Transform parent, Transform child, const Window& window) noexcept -> Transform;
} // namespace trc::ui
