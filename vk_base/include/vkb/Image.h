#pragma once

#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>

#include "VulkanBase.h"

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
     * @brief An image
     *
     * An image will always have the additional usage bit
     * `vk::ImageUsageFlagBits::eTransferSrc` set.
     */
    class Image
    {
    public:
        /**
         * A subresource range for one color image with one array layer
         * and one mipmap level.
         */
        static constexpr vk::ImageSubresourceRange DEFAULT_SUBRES_RANGE
            = { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 };

        /**
         * @brief Create an uninitialized image
         *
         * The underlying vk::Image is not created and no memory is
         * allocated.
         */
        Image() = default;

        /**
         * @brief Create an image
         *
         * Allocates memory for the image.
         *
         * @param const vk::ImageCreateInfo& info The standard Vulkan image
         *                                        create info struct.
         */
        explicit Image(const vk::ImageCreateInfo& info);

        /**
         * @brief Create a 2D image from a file
         *
         * Creates an image with one mip level and one array layer.
         */
        explicit Image(
            const std::string& imagePath,
            vk::ImageUsageFlags usage = vk::ImageUsageFlagBits::eSampled);

        explicit Image(glm::vec4 color,
                       vk::ImageUsageFlags usage = vk::ImageUsageFlagBits::eSampled);

        Image(const Image&) = delete;
        Image(Image&&) noexcept = default;
        ~Image() = default;

        auto operator=(const Image&) -> Image& = delete;
        auto operator=(Image&&) noexcept -> Image& = default;

        /**
         * @brief Access the underlying vk::Image handle
         *
         * @return vk::Image
         */
        auto operator*() const noexcept -> vk::Image;

        /**
         * @return The device memory backing the image
         */
        auto getMemory() const noexcept -> vk::DeviceMemory;

        auto getSize() const noexcept -> vk::Extent3D;
        auto getType() const noexcept -> vk::ImageType;
        auto getArrayLayerCount() const noexcept -> uint32_t;
        auto getMipLevelCount() const noexcept -> uint32_t;

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
         * @brief Create a new image from an image file source
         *
         * Creates a 2D image with one mip level and one array layer.
         * The format is vk::Format::aR8G8B8A8Uint.
         *
         * Image will be in layout vk::ImageLayout::eGeneral after this
         * operation.
         */
        void loadFromFile(const std::string& imagePath, vk::ImageUsageFlags usage);

        /**
         * @brief Copy raw data into a region of the image
         *
         * The user is responsible for providing the data in the correct
         * format for the image. Performs rudimentary boundary checks. The
         * user also has to bring the image in the correct layout.
         */
        void copyRawData(void* data, size_t size, ImageSize copySize);

        /**
         * @brief Create a view on the image
         */
        auto createView(vk::ImageViewType viewType,
                        vk::Format viewFormat,
                        vk::ComponentMapping componentMapping = {},
                        vk::ImageSubresourceRange subRes = DEFAULT_SUBRES_RANGE
            ) const -> vk::UniqueImageView;

        /**
         * Change the image's layout in a dedicated command buffer
         */
        void changeLayout(vk::ImageLayout from, vk::ImageLayout to,
                          vk::ImageSubresourceRange subRes = DEFAULT_SUBRES_RANGE);

        /**
         * Record the image layout change to a command buffer
         */
        void changeLayout(vk::CommandBuffer cmdBuf,
                          vk::ImageLayout from, vk::ImageLayout to,
                          vk::ImageSubresourceRange subRes = DEFAULT_SUBRES_RANGE);

    private:
        void recreateImage(const vk::ImageCreateInfo& info);

        /**
         * @brief Turn UINT32_MAXs into the image's extent
         *
         * @return vk::Extent3D The correct extent for the image
         */
        auto expandExtent(vk::Extent3D otherExtent) -> vk::Extent3D;

        vk::UniqueImage image;
        vk::UniqueDeviceMemory memory;

        vk::ImageType type;
        vk::Extent3D extent;
        uint32_t arrayLayers;
        uint32_t mipLevels;

        // I don't like mutable but I wanted getDefaultSampler() to be const
        mutable vk::UniqueSampler defaultSampler;
    };
} // namespace vkb
