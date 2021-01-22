#pragma once

#include "Types.h"

namespace trc::ui
{
    using SizeVal = vec2;

    enum class SizeType : ui8
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
            SizeType type{ SizeType::eNorm };
            Align alignment{ Align::eRelative };
        };

        SizeVal position{ 0.0f, 0.0f };
        SizeVal size{ 1.0f, 1.0f };

        Properties posProp{};
        Properties sizeProp{};
    };
} // namespace trc::ui
