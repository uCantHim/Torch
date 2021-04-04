#pragma once

#include <unordered_map>

#include "GlyphMap.h"

namespace trc
{
    struct GlyphDrawData
    {
        vec2 texCoordLL; // lower left texture coordinate
        vec2 texCoordUR; // upper right texture coordinate

        vec2 size;

        float bearingY;
        float advance;
    };

    class Font
    {
    public:
        explicit Font(const fs::path& path, ui32 fontSize = 18);

        /**
         * @brief Retrieve information about a glyph from the font
         *
         * Loads previously unused glyphs lazily.
         */
        auto getGlyph(CharCode charCode) -> GlyphDrawData;

        /**
         * @return float The amount of space between vertical lines of text
         */
        auto getLineBreakAdvance() const noexcept -> float;

        auto getDescriptor() const -> const GlyphMapDescriptor&;

    private:
        Face face;
        GlyphMap glyphMap;
        GlyphMapDescriptor descriptor;

        // Meta
        float lineBreakAdvance;

        std::unordered_map<CharCode, GlyphDrawData> glyphs;
    };
} // namespace trc
