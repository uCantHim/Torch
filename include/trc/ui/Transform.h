#pragma once

#include "Types.h"

namespace trc::ui
{
    enum class Format : ui8
    {
        eNorm,
        ePixel,
    };

    enum class Align : ui8
    {
        eRelative,
        eAbsolute
    };

    struct Transform
    {
        struct Properties
        {
            Format format{ Format::eNorm };
            Align alignment{ Align::eRelative };
        };

        vec2 position{ 0.0f, 0.0f };
        vec2 size{ 1.0f, 1.0f };

        Properties posProp{};
        Properties sizeProp{};
    };

    class Window;

    auto concat(Transform parent, Transform child, const Window& window) noexcept -> Transform;
} // namespace trc::ui
