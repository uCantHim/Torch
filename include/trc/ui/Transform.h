#pragma once

#include "Types.h"

namespace trc::ui
{
    using SizeVal = vec2;

    enum class Size : ui8
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
            Size sizeType{ Size::eNorm };
            Align alignment{ Align::eRelative };
        };

        SizeVal position;
        SizeVal size;

        Properties posProp{};
        Properties sizeProp{};
    };

    inline auto concat(Transform parent, Transform child) -> Transform
    {
        return {
            .position = child.posProp.alignment == Align::eRelative
                ? parent.position + child.position
                : child.position,
            .size = child.sizeProp.alignment == Align::eRelative
                ? parent.size * child.size
                : child.size
        };
    }
} // namespace trc::ui
