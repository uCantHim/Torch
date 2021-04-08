#pragma once

#include "Types.h"
#include "Transform.h"
#include "Style.h"
#include "text/GlyphLoading.h"

namespace trc::ui
{
    /**
     * Typed draw information
     */
    namespace types
    {
        struct NoType {};

        struct Line
        {
            ui32 width{ 1 };
        };

        struct Quad {};

        struct LetterInfo
        {
            // Unicode character code
            CharCode characterCode;

            // Offset from text position
            vec2 glyphOffset;
            vec2 glyphSize;

            // The glyph's bearing. Included here so that the
            // implementation doesn't have to look it up.
            float bearingY;
        };

        struct Text
        {
            ui32 fontIndex;
            std::vector<LetterInfo> letters;
        };
    }


    using DrawType = std::variant<
        types::NoType,
        types::Line,
        types::Quad,
        types::Text
    >;


    /**
     * All draw information for a gui element
     */
    struct DrawInfo
    {
        vec2 pos;
        vec2 size;
        ElementStyle style;
        DrawType type;
    };

    using DrawList = std::vector<DrawInfo>;

    class Drawable
    {
    protected:
        friend class Window;

        virtual ~Drawable() = default;

        virtual void draw(DrawList& drawList) = 0;
    };
} // namespace trc::ui
