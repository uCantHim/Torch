#pragma once

#include <memory>
#include <functional>
#include <filesystem>
namespace fs = std::filesystem;

#include <ft2build.h>
#include FT_FREETYPE_H

#include "../Types.h"
#include "trc_util/data/IndexMap.h"

namespace trc
{
    using CharCode = ui64;

    /**
     * @brief Metadata and pixel data of a font glyph
     */
    struct GlyphMeta
    {
        struct PixelData
        {
            ivec2 size;
            ui32 bearingY;
            ui32 negBearingY;
            ui32 advance;
        };
        struct NormalData
        {
            vec2 size;
            float bearingY;
            float negBearingY;
            float advance;
        };

        PixelData metaInPixels;
        NormalData metaNormalized;

        std::pair<std::vector<ui8>, uvec2> pixelData;
    };

    class Face
    {
    public:
        explicit Face(const fs::path& path, ui32 pixelSize = 40);

        auto loadGlyph(CharCode charCode) const -> GlyphMeta;

    private:
        std::unique_ptr<FT_Face, std::function<void(FT_Face*)>> face;
        FT_Face _face{ *face };

        inline auto scaleDevUnitsX(auto val)
        {
            if (FT_IS_SCALABLE(_face)) {
                return FT_MulFix(val, _face->size->metrics.x_scale);
            }
            return val;
        }

        inline auto scaleDevUnitsY(auto val)
        {
            if (FT_IS_SCALABLE(_face)) {
                return FT_MulFix(val, _face->size->metrics.y_scale);
            }
            return val;
        }

    public:
        const ui32 renderSize; // Pixel size in which the glyph's bitmaps are rendered

        const ui32 maxGlyphHeight; // Height of the highest glyph in pixels
        const ui32 maxGlyphWidth;  // Width of the widest glyph in pixels
        const ui32 lineSpace;      // Vertical space between lines of text in pixels

        const ui32 maxAscend;      // In pixels
        const i32 maxDescend;      // In pixels; can be negative
        const ui32 maxLineHeight;  // In pixels; equal to maxGlyphHeight

        const float lineSpaceNorm;     // Normalized vertical space between lines of text
        const float maxAscendNorm;     // Max glyph ascend from baseline relative to max glyph height
        const float maxDescendNorm;    // Max glyph descend from baseline relative to max glyph height
        const float maxLineHeightNorm; // Is always 1.0f
    };

    /**
     * @brief Loads glyphs lazily and caches them for repeated retrieval
     */
    class GlyphCache
    {
    public:
        explicit GlyphCache(Face face);

        /**
         * Queries glyph data from cache or, if it is not present in the
         * cache, loads the data from a face.
         *
         * @param CharCode character The character for which to get its
         *                           glyph.
         *
         * @return const GlyphMeta& Glyph data for the specified character
         */
        auto getGlyph(CharCode character) -> const GlyphMeta&;

        auto getFace() const noexcept -> const Face&;

    private:
        Face face;
        data::IndexMap<CharCode, u_ptr<GlyphMeta>> glyphs;
    };

    /**
     * Experimental! Does not work properly.
     */
    class SignedDistanceFace
    {
    public:
        SignedDistanceFace(const fs::path& path, ui32 fontSize);

        auto loadGlyphBitmap(CharCode charCode) const -> GlyphMeta;

    private:
        static constexpr ui32 RESOLUTION_FACTOR{ 32 };

        Face face;
        Face highresFace;
    };
} // namespace trc
