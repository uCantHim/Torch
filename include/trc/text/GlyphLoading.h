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
        ui32 maxGlyphHeight;
        ui32 maxGlyphWidth;
    };

    using CharCode = ui64;

    struct GlyphMeta
    {
        vec2 size;
        float bearingY;
        float negBearingY;
        float advance;

        std::pair<std::vector<ui8>, uvec2> pixelData;
    };

    auto loadGlyphBitmap(FT_Face face, CharCode charCode) -> GlyphMeta;
} // namespace trc
