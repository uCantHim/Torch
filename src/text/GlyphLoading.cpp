#include "text/GlyphLoading.h"



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
    face(new FT_Face, [](FT_Face* face) {
        FT_Done_Face(*face);
        delete face;
    })
{
    if (!fs::is_regular_file(path)) {
        throw std::runtime_error("Unable to load face: " + path.string() + " is not a file");
    }

    // Create face
    auto errorCode = FT_New_Face(getFreetype(), path.c_str(), 0, face.get());
    if (errorCode != 0) {
        throw std::runtime_error("Unable to load font (" + std::to_string(errorCode) + ")");
    }
    errorCode = FT_Set_Pixel_Sizes(*face, 0, fontSize);
    if (errorCode != 0) {
        throw std::runtime_error("Unable to load font with specific size "
                                 + std::to_string(fontSize) + ": " + std::to_string(errorCode));
    }

    maxGlyphWidth = ((*face)->bbox.xMax >> 6) - ((*face)->bbox.xMin >> 6);
    maxGlyphHeight = ((*face)->bbox.yMax >> 6) - ((*face)->bbox.yMin >> 6);
}

auto trc::loadGlyphBitmap(FT_Face face, CharCode charCode) -> GlyphMeta
{
    if (FT_Load_Char(face, charCode, FT_LOAD_RENDER) != 0) {
        throw std::runtime_error("Failed to load character '" + std::to_string(charCode));
    }

    const auto& glyphData = *face->glyph;
    const ui32 bitmapWidth = glyphData.bitmap.width;
    const ui32 bitmapHeight = glyphData.bitmap.rows;

    std::vector<ui8> data(bitmapWidth * bitmapHeight);
    memcpy(data.data(), glyphData.bitmap.buffer, data.size());

    const float maxWidth = static_cast<float>((face->bbox.xMax >> 6) - (face->bbox.xMin >> 6));
    const float maxHeight = static_cast<float>((face->bbox.yMax >> 6) - (face->bbox.yMin >> 6));

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
