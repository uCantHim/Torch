#pragma once

#include <string>

#include "trc/ui/Element.h"
#include "trc/ui/FontRegistry.h"
#include "trc/ui/Window.h"

namespace trc::ui
{
    /**
     * @brief Generate glyph positions for a string of text
     *
     * Always adds a trailing null-character at the end. It has the correct
     * position that a normal character would have. It can be used to
     * compute things that require the advance of the last character in
     * the string.
     *
     * @param vec2 scaling The function uses the font's normalized glyph
     *        data. Set this scaling to your desired font size divided by
     *        your window size to get properly sized glyphs.
     */
    auto layoutText(const std::string& str, ui32 fontIndex, vec2 scaling)
        -> std::pair<types::Text, vec2>;

    /**
     * @brief Generate glyph positions for an array of unicode characters
     *
     * Always adds a trailing null-character at the end. It has the correct
     * position that a normal character would have. It can be used to
     * compute things that require the advance of the last character in
     * the string.
     *
     * @param vec2 scaling The function uses the font's normalized glyph
     *        data. Set this scaling to your desired font size divided by
     *        your window size to get properly sized glyphs.
     */
    auto layoutText(const std::vector<CharCode>& chars, ui32 fontIndex, vec2 scaling)
        -> std::pair<types::Text, vec2>;

    /**
     * @return float The maximum glyph height of a font in screen coordinates
     */
    auto getFontHeight(ui32 fontIndex) -> float;

    /**
     * @return float The maximum glyph height of a font in pixels
     */
    auto getFontHeightPixels(ui32 fontIndex) -> float;

    /**
     * Calculate a more realistic (but heuristic!) value for a font's
     * height when using standard latin letters.
     *
     * Loads two glyphs into memory.
     *
     * @return float The maximum glyph height of a font in screen coordinates
     */
    auto getFontHeightLatin(ui32 fontIndex) -> float;

    /**
     * Calculate a more realistic (but heuristic!) value for a font's
     * height when using standard latin letters.
     *
     * Loads two glyphs into memory.
     *
     * @return float The maximum glyph height of a font in pixels
     */
    auto getFontHeightLatinPixels(ui32 fontIndex) -> float;

    class Text : public Element
    {
    public:
        explicit Text(Window& window);
        Text(Window& window,
             std::string str,
             ui32 fontIndex = DefaultStyle::font,
             ui32 fontSize = DefaultStyle::fontSize);

        void draw(DrawList&) override {}

        void print(std::string str);

    private:
        class Letters : public Element
        {
        public:
            explicit Letters(Window& window);

            void draw(DrawList& drawList) override;

            void setText(types::Text newText);

        private:
            types::Text text;
        };

        std::string printedText;
        UniqueElement<Letters> textElem;
    };
} // namespace trc::ui
