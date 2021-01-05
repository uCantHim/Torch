#pragma once

#include <glm/glm.hpp>

#include "Memory.h"

namespace vkb
{
    /**
     * @property subres    The subresource of an image. Defaults:
     *                      - Color aspect
     *                      - Miplevel 0
     *                      - Array layer 0
     * @property dstOffet  An offset into an image.
     * @property dstExtent The size of an image. If any component is
     *                     UINT32_MAX, that component signals that the
     *                     full size of the image is used.
     */
    struct ImageSize
    {
        vk::ImageSubresourceLayers subres{ vk::ImageAspectFlagBits::eColor, 0, 0, 1 };
        vk::Offset3D offset{ 0 };
        vk::Extent3D extent{ UINT32_MAX, UINT32_MAX, UINT32_MAX };
    };

    /**
     * A subresource range for one color image with one array layer
     * and one mipmap level.
     */
    constexpr vk::ImageSubresourceRange DEFAULT_SUBRES_RANGE{
        vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1
    };

    class Image
    {
    public:
        /**
         * @brief Incomplete initializating constructor
         *
         * Does not allocate memory, nor create an image. Actually, doesn't
         * call any Vulkan API functions at all.
         */
        Image() = default;

        /**
         * Image is left in vk::ImageLayout::eUndefined.
         */
        Image(const vk::ImageCreateInfo& createInfo,
              const DeviceMemoryAllocator& allocator = DefaultDeviceMemoryAllocator());

        Image(const Device& device,
              const vk::ImageCreateInfo& createInfo,
              const DeviceMemoryAllocator& allocator = DefaultDeviceMemoryAllocator());

        Image(Image&&) noexcept = default;
        auto operator=(Image&&) noexcept -> Image& = default;
        ~Image() = default;

        Image(const Image&) = delete;
        auto operator=(const Image&) -> Image& = delete;

        /**
         * @brief Access the underlying vk::Image handle
         *
         * @return vk::Image
         */
        auto operator*() const noexcept -> vk::Image;

        auto getSize() const noexcept -> vk::Extent3D;

        void changeLayout(const Device& device,
                          vk::ImageLayout newLayout,
                          vk::ImageSubresourceRange subRes = DEFAULT_SUBRES_RANGE);

        void changeLayout(vk::CommandBuffer cmdBuf,
                          vk::ImageLayout newLayout,
                          vk::ImageSubresourceRange subRes = DEFAULT_SUBRES_RANGE);

        /**
         * Change the image's layout in a dedicated command buffer
         */
        void changeLayout(const Device& device,
                          vk::ImageLayout from, vk::ImageLayout to,
                          vk::ImageSubresourceRange subRes = DEFAULT_SUBRES_RANGE);

        /**
         * Record the image layout change to a command buffer
         */
        void changeLayout(vk::CommandBuffer cmdBuf,
                          vk::ImageLayout from, vk::ImageLayout to,
                          vk::ImageSubresourceRange subRes = DEFAULT_SUBRES_RANGE);

        void writeData(const void* srcData, size_t srcSize, ImageSize destArea);

        /**
         * @brief Get a sampler object with default values for the image
         *
         * The image creates the sampler layzily on the first call to this
         * method.
         *
         * @return vk::Sampler
         */
        auto getDefaultSampler() const -> vk::Sampler;

        /**
         * @brief Create a view on the image
         */
        auto createView(vk::ImageViewType viewType,
                        vk::Format viewFormat,
                        vk::ComponentMapping componentMapping = {},
                        vk::ImageSubresourceRange subRes = DEFAULT_SUBRES_RANGE
            ) const -> vk::UniqueImageView;

    private:
        /**
         * @brief Turn UINT32_MAXs into the image's extent
         * @return vk::Extent3D The correct extent for the image
         */
        auto expandExtent(vk::Extent3D otherExtent) -> vk::Extent3D;

        void createNewImage(const Device& device,
                            const vk::ImageCreateInfo& createInfo,
                            const DeviceMemoryAllocator& allocator);

        DeviceMemory memory;
        vk::UniqueImage image;
        mutable vk::UniqueSampler defaultSampler;

        vk::ImageLayout currentLayout;
        vk::Extent3D size;
    };

    /**
     * @brief Make a 1x1 2D image with just one color
     */
    extern auto makeImage2D(glm::vec4 color,
                            vk::ImageUsageFlags usage = vk::ImageUsageFlagBits::eSampled
                                                        | vk::ImageUsageFlagBits::eTransferDst,
                            const DeviceMemoryAllocator& allocator = DefaultDeviceMemoryAllocator()
        ) -> Image;

    /**
     * @brief Make a 1x1 2D image with just one color
     */
    extern auto makeImage2D(const Device& device,
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
    extern auto makeImage2D(const fs::path& filePath,
                            vk::ImageUsageFlags usage = vk::ImageUsageFlagBits::eSampled
                                                        | vk::ImageUsageFlagBits::eTransferDst,
                            const DeviceMemoryAllocator& allocator = DefaultDeviceMemoryAllocator()
        ) -> Image;

    /**
     * @brief Load an image from file
     *
     * The created image is in the format r8g8b8a8.
     */
    extern auto makeImage2D(const Device& device,
                            const fs::path& filePath,
                            vk::ImageUsageFlags usage = vk::ImageUsageFlagBits::eSampled
                                                        | vk::ImageUsageFlagBits::eTransferDst,
                            const DeviceMemoryAllocator& allocator = DefaultDeviceMemoryAllocator()
        ) -> Image;
}
