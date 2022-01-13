#pragma once

#include <vkb/FrameSpecificObject.h>

#include "../Types.h"

namespace vkb {
    class Swapchain;
}

namespace trc
{
    /**
     * How do I communicate swapchain recreates where the vk::Image handles
     * are invalidated?
     *
     * Should the render target be responsible for acquiring and presenting
     * images?
     *     --> Probably not, since it might not even be a swapchain image
     */

    class RenderTarget;

    auto makeRenderTarget(const vkb::Swapchain& swapchain) -> RenderTarget;

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

        RenderTarget(const vkb::FrameClock& frameClock,
                     const vk::ArrayProxy<const vk::Image>& images,
                     const vk::ArrayProxy<const vk::ImageView>& imageViews,
                     const RenderTargetCreateInfo& createInfo);

        auto getFrameClock() const -> const vkb::FrameClock&;

        auto getCurrentImage() const -> vk::Image;
        auto getCurrentImageView() const -> vk::ImageView;

        auto getImage(ui32 frameIndex) const -> vk::Image;
        auto getImageView(ui32 frameIndex) const -> vk::ImageView;

        auto getImages() const -> const vkb::FrameSpecific<vk::Image>&;
        auto getImageViews() const -> const vkb::FrameSpecific<vk::ImageView>&;

        auto getSize() const -> uvec2;
        auto getFormat() const -> vk::Format;
        auto getImageUsage() const -> vk::ImageUsageFlags;

    private:
        vkb::FrameSpecific<vk::Image> images;
        vkb::FrameSpecific<vk::ImageView> imageViews;
        uvec2 size;
        vk::Format format;
        vk::ImageUsageFlags usage;
    };
} // namespace trc
