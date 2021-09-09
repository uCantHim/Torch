#pragma once

#include <glm/glm.hpp>

#include "Image.h"

namespace vkb
{
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
} // namespace vkb
