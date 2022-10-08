#include "trc/base/ImageUtils.h"

#define cimg_display 0
#define cimg_use_png 1
#include <CImg.h>
namespace cimg = cimg_library;



auto trc::makeSinglePixelImageData(glm::vec4 color) -> RawImageData
{
    glm::u8vec4 normalizedUnsigned{
        static_cast<uint8_t>(color.r * 128.0f),
        static_cast<uint8_t>(color.g * 128.0f),
        static_cast<uint8_t>(color.b * 128.0f),
        static_cast<uint8_t>(color.a * 128.0f),
    };

    return { { 1, 1 }, { normalizedUnsigned } };
}

auto trc::makeSinglePixelImage(
    const Device& device,
    glm::vec4 color,
    vk::ImageUsageFlags usage,
    const DeviceMemoryAllocator& allocator) -> Image
{
    Image result{
        device,
        vk::ImageCreateInfo(
            {}, vk::ImageType::e2D, vk::Format::eR8G8B8A8Unorm,
            { 1, 1, 1 }, 1, 1,
            vk::SampleCountFlagBits::e1, vk::ImageTiling::eOptimal, usage
        ),
        allocator
    };

    auto data = makeSinglePixelImageData(color);
    result.writeData(data.pixels.data(), sizeof(uint8_t) * 4, {});

    return result;
}

auto trc::loadImageData2D(const fs::path& filePath) -> RawImageData
{
    if (!fs::is_regular_file(filePath)) {
        throw std::runtime_error(filePath.string() + " is not a file");
    }

    cimg::CImg<uint8_t> image(filePath.c_str());
    if (image.spectrum() == 3)  // Image does not have an alpha channel
    {
        image.channels(0, 3);
        image.get_shared_channel(3).fill(255);
    }

    const uint32_t width = image.width();
    const uint32_t height = image.height();

    std::vector<glm::u8vec4> pixelData(width * height);
    for (size_t h = 0; h < height; h++)
    {
        for (size_t w = 0; w < width; w++)
        {
            const size_t offset = h * width + w;
            pixelData[offset][0] = image(w, h, 0, 0);
            pixelData[offset][1] = image(w, h, 0, 1);
            pixelData[offset][2] = image(w, h, 0, 2);
            pixelData[offset][3] = image(w, h, 0, 3);
        }
    }

    return {
        .size   = { width, height },
        .pixels = std::move(pixelData)
    };
}

auto trc::loadImage2D(
    const Device& device,
    const fs::path& filePath,
    vk::ImageUsageFlags usage,
    const DeviceMemoryAllocator& allocator) -> Image
{
    const auto data = loadImageData2D(filePath);
    const uint32_t width = data.size.x;
    const uint32_t height = data.size.y;

    Image result{ device, width, height, vk::Format::eR8G8B8A8Unorm, usage, allocator };
    result.writeData(data.pixels.data(), 4 * width * height, {});

    return result;
}
