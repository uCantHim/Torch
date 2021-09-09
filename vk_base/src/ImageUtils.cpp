#include "ImageUtils.h"

#include <IL/il.h>



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

    ILuint imageId = ilGenImage();
    ilBindImage(imageId);
    ilEnable(IL_ORIGIN_SET);
    ilOriginFunc(IL_ORIGIN_LOWER_LEFT);
    ILboolean success = ilLoadImage(filePath.c_str());
    if (!success)
    {
        ilDeleteImage(imageId);
        throw std::runtime_error("Unable to load image from \"" + filePath.string() + "\":"
                                 + std::to_string(ilGetError()));
    }
    ilConvertImage(IL_RGBA, IL_UNSIGNED_BYTE);

    const auto imageWidth = static_cast<uint32_t>(ilGetInteger(IL_IMAGE_WIDTH));
    const auto imageHeight = static_cast<uint32_t>(ilGetInteger(IL_IMAGE_HEIGHT));
    Image result{
        device,
        vk::ImageCreateInfo(
            {}, vk::ImageType::e2D, vk::Format::eR8G8B8A8Unorm,
            { imageWidth, imageHeight, 1 }, 1, 1,
            vk::SampleCountFlagBits::e1, vk::ImageTiling::eOptimal, usage
        ),
        allocator
    };

    result.writeData(ilGetData(), 4 * imageWidth * imageHeight, {});

    return result;
}
