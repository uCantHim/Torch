#include "Glyph.h"

#include <chrono>
using namespace std::chrono;
#include <iostream>



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
    //memcpy(data.data(), glyphData.bitmap.buffer, data.size());
    for (uint y = 0; y < bitmapHeight; y++)
    {
        const uint index = y * bitmapWidth;
        // Seems to be equivalent to index. Idk why I did this.
        //const uint bitmapIndex = ((bitmapHeight - 1) * bitmapWidth - (y * bitmapWidth));
        for (uint x = 0; x < bitmapWidth; x++)
        {
            data[index + x] = glyphData.bitmap.buffer[index + x];
        }
    }

    //std::cout << "Loaded glyph '" << char(charCode) << "':\n"
    //    << "\tTotal size: " << normalSize.x * maxWidth << ", " << normalSize.y * maxHeight << "\n"
    //    << "\tSize: " << glyph.size.x << ", " << glyph.size.y << "\n"
    //    << "\tBearing: " << glyph.bearingY << "\n"
    //    << "\tNegative bearing: " << glyph.negBearingY << "\n"
    //    << "\tAdvance: " << glyph.advance << "\n\n";

    const float maxWidth = (face->bbox.xMax >> 6) - (face->bbox.xMin >> 6);
    const float maxHeight = (face->bbox.yMax >> 6) - (face->bbox.yMin >> 6);

    const vec2 normalSize{
        (glyphData.metrics.width >> 6) / maxWidth,
        (glyphData.metrics.height >> 6) / maxHeight
    };
    const float bearingY = glyphData.metrics.horiBearingY >> 6;

    return {
        .size=normalSize,
        .bearingY=bearingY / maxHeight,
        .negBearingY=normalSize.y - bearingY / maxHeight,
        .advance=static_cast<float>(glyphData.metrics.horiAdvance >> 6) / face->max_advance_width,
        .pixelData={ std::move(data), uvec2(bitmapWidth, bitmapHeight) }
    };
}



trc::GlyphMap::GlyphMap()
    :
    image(
        vkb::getDevice(),
        vk::ImageCreateInfo(
            {},
            vk::ImageType::e2D,
            vk::Format::eR8Uint,
            { MAP_WIDTH, MAP_HEIGHT, 1 },
            1, 1,
            vk::SampleCountFlagBits::e1,
            vk::ImageTiling::eOptimal,
            vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst
        ),
        memoryPool->makeAllocator()
    )
{
}

auto trc::GlyphMap::addGlyph(const GlyphMeta& glyph) -> UvRectangle
{
    const auto& [data, size] = glyph.pixelData;
    if (data.empty()) {
        return { { 0.0f, 0.0f }, { 0.0f, 0.0f } };
    }

    if (offset.x + size.x > MAP_WIDTH)
    {
        offset.x = 0;
        offset.y += maxHeight;
        maxHeight = 0;
    }
    if (offset.y + size.y > MAP_HEIGHT) {
        throw std::out_of_range("No more space in glyph map!");
    }

    image.writeData(
        data.data(),
        size.x * size.y,
        vkb::ImageSize{ .offset={ offset.x, offset.y, 0 }, .extent={ size.x, size.y, 1 } }
    );

    GLM_CONSTEXPR vec2 mapSize{ MAP_WIDTH, MAP_HEIGHT };
    vec2 ll{ vec2(offset) / mapSize };
    vec2 ur{ (vec2(offset) + vec2(size)) / mapSize };

    maxHeight = glm::max(maxHeight, size.y);
    offset.x += size.x;

    return { ll, ur };
}

auto trc::GlyphMap::getGlyphImage() -> vkb::Image&
{
    return image;
}
