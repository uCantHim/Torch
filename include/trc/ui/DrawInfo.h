#pragma once

#include <variant>
#include <vector>

#include "trc/Types.h"
#include "trc/text/GlyphLoading.h"
#include "trc/ui/Style.h"
#include "trc/ui/Transform.h"

namespace trc::ui
{
    /**
     * Typed draw information
     */
    namespace types
    {
        /**
         * TODO: I can implement thick lines as quads if I want to add
         * the line-width parameter again.
         */
        struct Line {};

        struct Quad {};

        struct Text
        {
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

            ui32 fontIndex;
            std::vector<LetterInfo> letters;
        };
    }

    /**
     * All draw information for a gui element
     */
    template<typename T>
    struct DrawInfo
    {
        vec2 pos;
        vec2 size;
        ElementStyle& style;
        T elem;
    };

    class DrawList;

    class Drawable
    {
    protected:
        friend class Window;

        virtual ~Drawable() = default;

        virtual void draw(DrawList& drawList) = 0;
    };
} // namespace trc::ui
