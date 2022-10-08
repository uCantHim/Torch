#include "trc/assets/import/PNGConvert.h"

#include <cstring>
#include <fstream>

#include <png.h>



namespace trc
{

bool isPNG(const fs::path& filePath)
{
    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }

    char buf[8];
    file.read(buf, 8);

    return isPNG(buf, file.gcount());
}

bool isPNG(const void* data, size_t size)
{
    size = std::min(size, size_t(8));
    const bool isPng = !png_sig_cmp(static_cast<png_const_bytep>(data), 0, size);
    return isPng;
}



struct PngWriteOutput
{
    ui32 offset{ 0 };
    std::vector<ui8> data;
};

struct PngReadInput
{
    ui32 offset{ 0 };
    png_const_bytep data;
};

auto toPNG(const TextureData& tex) -> std::vector<ui8>
{
    auto png = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
    if (png == nullptr) {
        throw std::runtime_error("Initialization of PNG failed");
    }

    auto info = png_create_info_struct(png);
    if (info == nullptr)
    {
        png_destroy_write_struct(&png, nullptr);
        throw std::runtime_error("Initialization of PNG info struct failed");
    }

    // Error handling
    if (setjmp(png_jmpbuf(png)))
    {
        png_destroy_write_struct(&png, &info);
        throw std::runtime_error("PNG encountered an error");
    }

    // Set up write function
    auto writeData = [](png_structp png, png_bytep data, png_size_t size)
    {
        auto out = (PngWriteOutput*)png_get_io_ptr(png);
        assert(out->offset == out->data.size());

        out->data.resize(out->offset + size);
        memcpy(out->data.data() + out->offset, data, size);
        out->offset += size;
    };
    auto flushData = [](png_structp) {};

    PngWriteOutput out;
    png_set_write_fn(png, &out, writeData, flushData);

    // Set metadata
    png_set_IHDR(png, info,
        tex.size.x, tex.size.y,
        8, // bit depth
        PNG_COLOR_TYPE_RGB_ALPHA,
        PNG_INTERLACE_NONE,
        PNG_COMPRESSION_TYPE_DEFAULT,
        PNG_FILTER_TYPE_DEFAULT
    );

    // Set input image data
    std::vector<png_bytep> rows(tex.size.y);
    for (ui32 i = 0; i < tex.size.y; i++)
    {
        rows[i] = (png_bytep)&tex.pixels[i * tex.size.x];
    }
    png_set_rows(png, info, rows.data());

    // Write
    png_write_png(png, info, PNG_TRANSFORM_IDENTITY, nullptr);

    // Terminate
    png_write_end(png, info);
    png_destroy_write_struct(&png, &info);

    return out.data;
}

auto fromPNG(const std::vector<ui8>& data) -> TextureData
{
    return fromPNG(data.data(), data.size());
}

auto fromPNG(const void* data, const size_t size) -> TextureData
{
    if (!isPNG(data, size)) {
        throw std::runtime_error("Unable to read PNG data: Input data is not PNG data!");
    }

    // Initialize
    auto png = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
    if (png == nullptr) {
        throw std::runtime_error("Initialization of PNG failed");
    }

    auto info = png_create_info_struct(png);
    if (info == nullptr)
    {
        png_destroy_read_struct(&png, nullptr, nullptr);
        throw std::runtime_error("Initialization of PNG info struct failed");
    }

    auto endInfo = png_create_info_struct(png);
    if (endInfo == nullptr)
    {
        png_destroy_read_struct(&png, &info, nullptr);
        throw std::runtime_error("Initialization of PNG info struct failed");
    }

    // Error handling
    if (setjmp(png_jmpbuf(png)))
    {
        png_destroy_read_struct(&png, &info, &endInfo);
        throw std::runtime_error("PNG encountered an error");
    }

    // Set up read function
    auto readData = [](png_structp png, png_bytep data, png_size_t size)
    {
        auto in = (PngReadInput*)png_get_io_ptr(png);

        memcpy(data, in->data + in->offset, size);
        in->offset += size;
    };

    PngReadInput in{ .offset=0, .data=static_cast<png_const_bytep>(data) };
    png_set_read_fn(png, &in, readData);

    // Read metadata
    png_read_info(png, info);

    const ui32 width = png_get_image_width(png, info);
    const ui32 height = png_get_image_height(png, info);
    assert(png_get_bit_depth(png, info) == 8);
    assert(png_get_channels(png, info) == 4);

    TextureData tex{
        .size={ width, height },
        .pixels={}
    };
    tex.pixels.resize(tex.size.x * tex.size.y);

    // Set row pointers to which data will be written
    std::vector<png_bytep> rowPtrs(height);
    for (ui32 i = 0; i < height; i++)
    {
        rowPtrs[i] = (png_bytep)&tex.pixels[i * width];
    }

    // Read image data
    png_read_image(png, rowPtrs.data());

    // Terminate
    png_read_end(png, endInfo);
    png_destroy_read_struct(&png, &info, &endInfo);

    return tex;
}

} // namespace trc
