#include "core/RenderTarget.h"

#include <vkb/Swapchain.h>



auto trc::makeRenderTarget(const vkb::Swapchain& swapchain) -> RenderTarget
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
        swapchain.getSize(),
        swapchain.getImageFormat()
    };
}



trc::RenderTarget::RenderTarget(
    const vkb::FrameClock& frameClock,
    const vk::ArrayProxy<const vk::Image>& images,
    const vk::ArrayProxy<const vk::ImageView>& imageViews,
    uvec2 imageSize,
    vk::Format imageFormat)
    :
    images(frameClock, { images.begin(), images.end() }),
    imageViews(frameClock, { imageViews.begin(), imageViews.end() }),
    size(imageSize),
    format(imageFormat)
{
}

auto trc::RenderTarget::getFrameClock() const -> const vkb::FrameClock&
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

auto trc::RenderTarget::getImages() const -> const vkb::FrameSpecific<vk::Image>&
{
    return images;
}

auto trc::RenderTarget::getImageViews() const -> const vkb::FrameSpecific<vk::ImageView>&
{
    return imageViews;
}
