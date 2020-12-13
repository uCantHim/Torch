#pragma once

#include <memory>
#include <functional>
#include <filesystem>
namespace fs = std::filesystem;

#include <ft2build.h>
#include FT_FREETYPE_H

#include <vkb/Image.h>
#include <vkb/MemoryPool.h>
#include <vkb/StaticInit.h>

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

    class GlyphMap
    {
    public:
        struct UvRectangle
        {
            vec2 lowerLeft;
            vec2 upperRight;
        };

        GlyphMap();

        /**
         * @param const GlyphMeta& glyph A new glyph
         *
         * @return UvRectangle UV coordinates of the new glyph in the map
         *
         * @throw std::out_of_range if there's no more space in the glyph
         *        texture
         */
        auto addGlyph(const GlyphMeta& glyph) -> UvRectangle;

        /**
         * @return vkb::Image& The image that contains all glyphs
         */
        auto getGlyphImage() -> vkb::Image&;

    private:
        static constexpr ui32 MAP_WIDTH{ 5000 };
        static constexpr ui32 MAP_HEIGHT{ 1000 };

        static inline std::unique_ptr<vkb::MemoryPool> memoryPool;
        static inline vkb::StaticInit _init{
            [] { memoryPool.reset(new vkb::MemoryPool(vkb::getDevice(), 25000000)); }
        };

        ivec2 offset{ 0, 0 };
        ui32 maxHeight{ 0 };
        vkb::Image image;
    };
} // namespace trc
