#pragma once

#include "Types.h"
#include "Transform.h"
#include "Style.h"

namespace trc::ui
{
    /**
     * Typed draw information
     */
    namespace types
    {
        struct NoType {};
        struct Quad {};

        struct LetterInfo
        {
            // Unicode character code
            wchar_t characterCode;

            // Offset from text position
            ivec2 glyphOffsetPixels;
            ivec2 glyphSizePixels;

            // The glyph's bearing. Included here so that the
            // implementation doesn't have to look it up.
            ui32 bearingYPixels;
        };

        struct Text
        {
            ui32 fontIndex;
            std::vector<LetterInfo> letters;
        };
    }


    using DrawType = std::variant<
        types::NoType,
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

        virtual void draw(DrawList& drawList, vec2 globalPos, vec2 globalSize) = 0;
    };
} // namespace trc::ui
