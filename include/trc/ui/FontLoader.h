#pragma once

#include <atomic>
#include <filesystem>
#include <string>

#include "trc/Types.h"
#include "trc/text/GlyphLoading.h"
#include "trc/ui/DrawInfo.h"

namespace trc::ui
{
    class FontLoader
    {
    public:
        virtual auto loadFont(const fs::path& file, ui32 fontSize) -> ui32 = 0;
        virtual auto getFontInfo(ui32 fontIndex) -> const Face& = 0;
        virtual auto getGlyph(ui32 fontIndex, CharCode character) -> const GlyphMeta& = 0;
    };

    class FontLayouter
    {
    public:
        explicit FontLayouter(FontLoader& backend);

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

    private:
        auto getFontInfo(ui32 fontIndex) -> const Face&;
        auto getGlyph(ui32 fontIndex, CharCode character) -> const GlyphMeta&;

        FontLoader* fontLoader;
    };
} // namespace trc::ui
