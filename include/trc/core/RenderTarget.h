#pragma once

#include <generator>

#include "trc/Types.h"
#include "trc/VulkanInclude.h"
#include "trc/base/FrameSpecificObject.h"

namespace trc
{
    class Swapchain;

    struct RenderImage
    {
        vk::Image image;
        vk::ImageView imageView;

        vk::Format format;
        vk::ImageUsageFlags possibleUsage;

        uvec2 size;
    };

    struct RenderArea
    {
        ivec2 offset;
        uvec2 size;
    };

    struct Viewport
    {
        RenderImage target;
        RenderArea area;
    };

    /**
     * @brief A reference to image resources to which Torch can draw.
     */
    class RenderTarget;

    /**
     * @brief Helper to create a render target that points to a swapchain.
     */
    auto makeRenderTarget(const Swapchain& swapchain) -> RenderTarget;

    /**
     * The `imageUsage` parameter is in many cases unnecessary, unless you
     * want to create an imageless framebuffer.
     */
    struct RenderTargetCreateInfo
    {
         uvec2 imageSize;
         vk::Format imageFormat;
         vk::ImageUsageFlags imageUsage;
    };

    class RenderTarget
    {
    public:
        RenderTarget(const RenderTarget&) = default;
        RenderTarget(RenderTarget&&) noexcept = default;
        auto operator=(const RenderTarget&) -> RenderTarget& = default;
        auto operator=(RenderTarget&&) noexcept -> RenderTarget& = default;
        ~RenderTarget() = default;

        RenderTarget(const FrameClock& frameClock,
                     const vk::ArrayProxy<const vk::Image>& images,
                     const vk::ArrayProxy<const vk::ImageView>& imageViews,
                     const RenderTargetCreateInfo& createInfo);

        auto getFrameClock() const -> const FrameClock&;

        auto getCurrentImage() const -> vk::Image;
        auto getCurrentImageView() const -> vk::ImageView;
        auto getCurrentRenderImage() const -> RenderImage;

        auto getImage(ui32 frameIndex) const -> vk::Image;
        auto getImageView(ui32 frameIndex) const -> vk::ImageView;
        auto getRenderImage(ui32 frameIndex) const -> RenderImage;

        auto getImages() const -> const FrameSpecific<vk::Image>&;
        auto getImageViews() const -> const FrameSpecific<vk::ImageView>&;
        auto getRenderImages() const -> std::generator<RenderImage>;

        auto getSize() const -> uvec2;
        auto getFormat() const -> vk::Format;
        auto getImageUsage() const -> vk::ImageUsageFlags;

    private:
        FrameSpecific<vk::Image> images;
        FrameSpecific<vk::ImageView> imageViews;
        uvec2 size;
        vk::Format format;
        vk::ImageUsageFlags usage;
    };
} // namespace trc
