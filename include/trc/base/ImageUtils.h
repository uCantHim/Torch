#pragma once

#include <filesystem>
#include <vector>

#include <glm/glm.hpp>

#include "trc/base/Image.h"

namespace trc
{
    namespace fs = std::filesystem;

    struct RawImageData
    {
        glm::uvec2 size;
        std::vector<glm::u8vec4> pixels;
    };

    auto makeSinglePixelImageData(glm::vec4 color) -> RawImageData;

    /**
     * @brief Make a 1x1 2D image with just one color
     */
    auto makeSinglePixelImage(const Device& device,
                              glm::vec4 color,
                              vk::ImageUsageFlags usage = vk::ImageUsageFlagBits::eSampled
                                                          | vk::ImageUsageFlagBits::eTransferDst,
                              const DeviceMemoryAllocator& allocator = DefaultDeviceMemoryAllocator()
        ) -> Image;

    /**
     * @throw std::runtime_error if something goes wrong.
     */
    auto loadImageData2D(const fs::path& filePath) -> RawImageData;

    /**
     * @brief Load an image from file
     *
     * The created image is in the format r8g8b8a8.
     */
    auto loadImage2D(const Device& device,
                     const fs::path& filePath,
                     vk::ImageUsageFlags usage = vk::ImageUsageFlagBits::eSampled
                                                 | vk::ImageUsageFlagBits::eTransferDst,
                     const DeviceMemoryAllocator& allocator = DefaultDeviceMemoryAllocator()
        ) -> Image;
} // namespace trc
