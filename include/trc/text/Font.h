#pragma once

#include <unordered_map>

#include "../core/DescriptorProvider.h"
#include "GlyphMap.h"

namespace trc
{
    class FontDataStorage;

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
        Font(FontDataStorage& storage, Face face);
        Font(FontDataStorage& storage, const fs::path& path, ui32 fontSize = 18);

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

        auto getDescriptor() const -> const DescriptorProvider&;

    private:
        Face face;
        GlyphMap* glyphMap;
        DescriptorProvider descProvider;

        // Meta
        float lineBreakAdvance;

        std::unordered_map<CharCode, GlyphDrawData> glyphs;
    };
} // namespace trc
