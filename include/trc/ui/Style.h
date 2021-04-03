#pragma once

#include <variant>

#include "Types.h"

namespace trc::ui
{
    struct TextureInfo
    {
        vec2 uvLL;
        vec2 uvUR;
        ui32 textureIndex;
    };

    class DefaultStyle
    {
    public:
        static inline vec4 background{ 0.3f, 0.3f, 0.7f, 1.0f };
        static inline vec4 textColor{ 1.0f };

        static inline ui32 borderThickness{ 0 };
        static inline vec4 borderColor{ 0.8f, 0.8f, 1.0f, 1.0f };
    };

    /**
     * Generic draw information for all elements
     */
    struct ElementStyle
    {
        std::variant<vec4, TextureInfo> background{ DefaultStyle::background };

        ui32 borderThickness{ DefaultStyle::borderThickness };
        vec4 borderColor{ DefaultStyle::borderColor };
    };
} // namespace trc::ui
