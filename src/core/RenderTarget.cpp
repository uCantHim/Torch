#include "trc/core/RenderTarget.h"

#include "trc/base/Swapchain.h"



auto trc::makeRenderTarget(const Swapchain& swapchain) -> RenderTarget
{
    std::vector<vk::Image> images;
    std::vector<vk::ImageView> imageViews;
    for (ui32 i = 0; i < swapchain.getFrameCount(); i++)
    {
        images.emplace_back(swapchain.getImage(i));
        imageViews.emplace_back(swapchain.getImageView(i));
    }

    return {
        swapchain,
        std::move(images),
        std::move(imageViews),
        {
            swapchain.getSize(),
            swapchain.getImageFormat(),
            swapchain.getImageUsage()
        }
    };
}



trc::RenderTarget::RenderTarget(
    const FrameClock& frameClock,
    const vk::ArrayProxy<const vk::Image>& images,
    const vk::ArrayProxy<const vk::ImageView>& imageViews,
    const RenderTargetCreateInfo& info)
    :
    images(frameClock, { images.begin(), images.end() }),
    imageViews(frameClock, { imageViews.begin(), imageViews.end() }),
    size(info.imageSize),
    format(info.imageFormat),
    usage(info.imageUsage)
{
}

auto trc::RenderTarget::getFrameClock() const -> const FrameClock&
{
    return images.getFrameClock();
}

auto trc::RenderTarget::getCurrentImage() const -> vk::Image
{
    return images.get();
}

auto trc::RenderTarget::getCurrentImageView() const -> vk::ImageView
{
    return imageViews.get();
}

auto trc::RenderTarget::getImage(ui32 frameIndex) const -> vk::Image
{
    return images.getAt(frameIndex);
}

auto trc::RenderTarget::getImageView(ui32 frameIndex) const -> vk::ImageView
{
    return imageViews.getAt(frameIndex);
}

auto trc::RenderTarget::getImages() const -> const FrameSpecific<vk::Image>&
{
    return images;
}

auto trc::RenderTarget::getImageViews() const -> const FrameSpecific<vk::ImageView>&
{
    return imageViews;
}

auto trc::RenderTarget::getSize() const -> uvec2
{
    return size;
}

auto trc::RenderTarget::getFormat() const -> vk::Format
{
    return format;
}

auto trc::RenderTarget::getImageUsage() const -> vk::ImageUsageFlags
{
    return usage;
}
