#include "ImageUtils.h"

#define cimg_display 0
#define cimg_use_png 1
#include <CImg.h>
namespace cimg = cimg_library;



auto vkb::makeSinglePixelImage(
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

    // Why does R8G8B8A8Unorm assume that the supplied data is in signed integer format?
    std::array<uint8_t, 4> normalizedUnsigned{
        static_cast<uint8_t>(color.r * 128.0f),
        static_cast<uint8_t>(color.g * 128.0f),
        static_cast<uint8_t>(color.b * 128.0f),
        static_cast<uint8_t>(color.a * 128.0f),
    };
    result.writeData(normalizedUnsigned.data(), sizeof(uint8_t) * 4, {});

    return result;
}

auto vkb::loadImage2D(
    const Device& device,
    const fs::path& filePath,
    vk::ImageUsageFlags usage,
    const DeviceMemoryAllocator& allocator) -> Image
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

    uint8_t* pixelData = new uint8_t[4 * image.width() * image.height()];
    for (size_t h = 0; h < height; h++)
    {
        for (size_t w = 0; w < width; w++)
        {
            const size_t offset = 4 * h * width + 4 * w;
            pixelData[offset + 0] = image(w, h, 0, 0);
            pixelData[offset + 1] = image(w, h, 0, 1);
            pixelData[offset + 2] = image(w, h, 0, 2);
            pixelData[offset + 3] = image(w, h, 0, 3);
        }
    }

    Image result{ device, width, height, vk::Format::eR8G8B8A8Unorm, usage, allocator };
    result.writeData(pixelData, 4 * width * height, {});
    delete[] pixelData;

    return result;
}
