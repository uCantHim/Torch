#include "text/GlyphLoading.h"

#include <glm/glm.hpp>



inline auto getFreetype() -> FT_Library
{
    // Initialize freetype
    static FT_Library ft = [] {
        FT_Library ft;
        auto errorCode = FT_Init_FreeType(&ft);
        if (errorCode != 0) {
            throw std::runtime_error("Unable to initialize freetype ("
                                     + std::to_string(errorCode) + ")");
        }

        return ft;
    }();

    return ft;
}



trc::Face::Face(const fs::path& path, ui32 fontSize)
    :
    face(
        // Create the face
        [&] {
            FT_Face* face = new FT_Face;

            if (!fs::is_regular_file(path)) {
                throw std::runtime_error("Unable to load face: " + path.string() + " is not a file");
            }

            // Create face
            auto errorCode = FT_New_Face(getFreetype(), path.c_str(), 0, face);
            if (errorCode != 0) {
                throw std::runtime_error("Unable to load font (" + std::to_string(errorCode) + ")");
            }
            errorCode = FT_Set_Pixel_Sizes(*face, 0, fontSize);
            if (errorCode != 0) {
                throw std::runtime_error("Unable to load font with specific size "
                                         + std::to_string(fontSize) + ": " + std::to_string(errorCode));
            }
            errorCode = FT_Select_Charmap(*face, FT_ENCODING_UNICODE);
            if (errorCode != 0) {
                throw std::runtime_error("Unable to select unicode encoding on FreeType face");
            }
            return face;
        }(),
        // Deleter
        [](FT_Face* face) {
            FT_Done_Face(*face);
            delete face;
        }
    ),
    maxGlyphHeight(((*face)->bbox.yMax >> 6) - ((*face)->bbox.yMin >> 6)),
    maxGlyphWidth(((*face)->bbox.xMax >> 6) - ((*face)->bbox.xMin >> 6)),
    // maxGlyphHeight and lineSpace seem to be the same values
    lineSpace(((*face)->size->metrics.height >> 6))
{
}

auto trc::Face::loadGlyph(CharCode charCode) const -> GlyphMeta
{
    if (FT_Load_Char(*face, charCode, FT_LOAD_RENDER) != 0) {
        throw std::runtime_error("Failed to load character '" + std::to_string(charCode));
    }

    const auto& glyphData = *(*face)->glyph;
    const ui32 bitmapWidth = glyphData.bitmap.width;
    const ui32 bitmapHeight = glyphData.bitmap.rows;

    std::vector<ui8> data(bitmapWidth * bitmapHeight);
    memcpy(data.data(), glyphData.bitmap.buffer, data.size());

    const float maxWidth = static_cast<float>(((*face)->bbox.xMax >> 6) - ((*face)->bbox.xMin >> 6));
    const float maxHeight = static_cast<float>(((*face)->bbox.yMax >> 6) - ((*face)->bbox.yMin >> 6));

    const ivec2 pixelSize{ glyphData.metrics.width >> 6, glyphData.metrics.height >> 6 };
    const vec2 normalSize{ static_cast<vec2>(pixelSize) / vec2(maxWidth, maxHeight) };

    const ui32 bearingY = glyphData.metrics.horiBearingY >> 6;
    const ui32 advance = glyphData.metrics.horiAdvance >> 6;

    return {
        .metaInPixels = {
            .size        = pixelSize,
            .bearingY    = bearingY,
            .negBearingY = pixelSize.y - bearingY,
            .advance     = advance
        },
        .metaNormalized = {
            .size        = normalSize,
            .bearingY    = static_cast<float>(bearingY) / maxHeight,
            .negBearingY = normalSize.y - (static_cast<float>(bearingY) / maxHeight),
            .advance     = static_cast<float>(advance) / maxWidth,
        },
        .pixelData   = { std::move(data), uvec2(bitmapWidth, bitmapHeight) }
    };
}



trc::GlyphCache::GlyphCache(Face face)
    :
    face(std::forward<Face>(face))
{
}

auto trc::GlyphCache::getGlyph(CharCode character) -> const GlyphMeta&
{
    auto& glyph = glyphs[character];
    if (!glyph) {
        glyph = std::make_unique<GlyphMeta>(face.loadGlyph(character));
    }

    return *glyph;
}

auto trc::GlyphCache::getFace() const noexcept -> const Face&
{
    return face;
}



trc::SignedDistanceFace::SignedDistanceFace(const fs::path& path, ui32 fontSize)
    :
    face(path, fontSize),
    highresFace(path, fontSize * RESOLUTION_FACTOR)
{
}

auto trc::SignedDistanceFace::loadGlyphBitmap(CharCode charCode) const -> GlyphMeta
{
    const auto highres = highresFace.loadGlyph(charCode);
    auto lowres = face.loadGlyph(charCode);

    constexpr i32 neighborhoodRadius{ RESOLUTION_FACTOR * 3 };
    constexpr float maxDist{ neighborhoodRadius };
    const i32 width = lowres.pixelData.second.x;
    const i32 height = lowres.pixelData.second.y;
    const i32 highresWidth = highres.pixelData.second.x;
    const i32 highresHeight = highres.pixelData.second.y;
    for (i32 x = 0; x < width; x++)
    {
        for (i32 y = 0; y < height; y++)
        {
            const i32 highresX = x * RESOLUTION_FACTOR + (RESOLUTION_FACTOR * 0.5f);
            const i32 highresY = y * RESOLUTION_FACTOR + (RESOLUTION_FACTOR * 0.5f);
            ui8& current = lowres.pixelData.first.at(y * width + x);

            auto isIn = [](ui8 v) { return v > 128; };
            const bool currentIn = isIn(current);

            float currDist{ maxDist };
            for (i32 i = glm::max(0, highresX - neighborhoodRadius);
                 i < glm::min(highresWidth, highresX + neighborhoodRadius);
                 i++)
            {
                for (i32 j = glm::max(0, highresY - neighborhoodRadius);
                     j < glm::min(highresHeight, highresY + neighborhoodRadius);
                     j++)
                {
                    // Only try distance to pixel of other type
                    if (currentIn != isIn(highres.pixelData.first.at(j * highresWidth + i)))
                    {
                        currDist = glm::min(
                            currDist,
                            glm::distance(vec2(highresX, highresY), vec2(i, j))
                        );
                    }
                }
            }

            const float normDist = glm::min(1.0f, currDist / maxDist);
            // In this value, 0.5 is at the edge. 0 is maximum dist outside of edge,
            // 1 is maximum dist inside of edge.
            const float finalDist = currentIn
                ? 0.5f + 0.5f * normDist
                : 0.5f - 0.5f * normDist;

            current = 255 * finalDist;
        }
    }

    return lowres;
}
