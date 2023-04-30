#pragma once

#include <glm/glm.hpp>

#include "trc/base/Memory.h"

namespace trc
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
         * @brief Create a 2D image
         *
         * @param uint32_t width
         * @param uint32_t height
         * @param vk::Format format
         */
        Image(const trc::Device& device,
              uint32_t width,
              uint32_t height,
              vk::Format format = vk::Format::eR8G8B8A8Unorm,
              vk::ImageUsageFlags usage = vk::ImageUsageFlagBits::eSampled
                                        | vk::ImageUsageFlagBits::eStorage
                                        | vk::ImageUsageFlagBits::eTransferDst,
              const DeviceMemoryAllocator& alloc = DefaultDeviceMemoryAllocator());

        /**
         * Image is in layout vk::ImageLayout::eUndefined.
         */
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

        auto getType() const noexcept -> vk::ImageType;
        auto getFormat() const noexcept -> vk::Format;
        auto getSize() const noexcept -> glm::uvec2;
        auto getExtent() const noexcept -> vk::Extent3D;

        /**
         * Create a transient buffer for the image write and copy data to the
         * image's device-local memory.
         *
         * The image *must* be in `vk::ImageLayout::eTransferDstOptimal`.
         *
         * @param const DeviceMemoryAllocator& alloc An allocator for the
         *        transfer buffer's memory.
         */
        void writeData(const void* srcData,
                       size_t srcSize,
                       const ImageSize& destArea,
                       const DeviceMemoryAllocator& alloc = DefaultDeviceMemoryAllocator{});

        /**
         * The image *must* be in `vk::ImageLayout::eTransferDstOptimal`.
         */
        void writeData(vk::CommandBuffer cmdBuf,
                       vk::Buffer srcData,
                       const ImageSize& destArea);

        auto getMemory() const noexcept -> const DeviceMemory&;

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
         *
         * Simplified interface for the most common use case. Use the
         * image's format and type as the view's format and type.
         */
        auto createView(vk::ImageAspectFlags aspect = vk::ImageAspectFlagBits::eColor) const
            -> vk::UniqueImageView;

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

        const trc::Device* device;

        DeviceMemory memory;
        vk::UniqueImage image;
        mutable vk::UniqueSampler defaultSampler;

        vk::ImageType type;
        vk::Format format;
        vk::Extent3D extent;
    };
} // namespace trc
