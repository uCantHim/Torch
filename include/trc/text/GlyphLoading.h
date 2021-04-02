#pragma once

#include <memory>
#include <functional>
#include <filesystem>
namespace fs = std::filesystem;

#include <ft2build.h>
#include FT_FREETYPE_H

#include "Types.h"

namespace trc
{
    using FaceDestructor = std::function<void(FT_Face*)>;
    using UniqueFace = std::unique_ptr<FT_Face, FaceDestructor>;

    struct Face
    {
        explicit Face(const fs::path& path, ui32 fontSize = 18);

        UniqueFace face;

        ui32 maxGlyphHeight; // Height of the highest glyph in pixels
        ui32 maxGlyphWidth;  // Width of the widest glyph in pixels
        ui32 lineSpace;      // Space between lines of text in pixels
    };

    using CharCode = ui64;

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

    auto loadGlyphBitmap(FT_Face face, CharCode charCode) -> GlyphMeta;
    auto loadGlyphBitmap(const Face& face, CharCode charCode) -> GlyphMeta;
    auto loadGlyphBitmap(const SignedDistanceFace& face, CharCode charCode) -> GlyphMeta;
} // namespace trc
