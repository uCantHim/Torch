#include "Framebuffer.h"

#include <iostream>



Framebuffer::Framebuffer(const vk::RenderPass& renderpass)
    :
    colorImageViews(
        [&](uint32_t imageIndex) -> vk::ImageView {

            vk::ImageSubresourceRange subresRange(
                vk::ImageAspectFlagBits::eColor,
                0, 1, // Mip level and -count
                0, 1  // Array layer and -count
            );
            vk::ImageViewCreateInfo info(
                vk::ImageViewCreateFlags(),
                getSwapchain().getImage(imageIndex),
                vk::ImageViewType::e2D,
                getSwapchain().getImageFormat(),
                vk::ComponentMapping(), // Component mapping default is identity
                subresRange
            );

            return getDevice().get().createImageView(info);
        }
    ),
    framebuffers(
        [&](uint32_t imageIndex) -> vk::Framebuffer {
            vk::FramebufferCreateInfo info(
                vk::FramebufferCreateFlags(),
                renderpass,
                1, &colorImageViews.getAt(imageIndex),
                getSwapchain().getImageExtent().width, getSwapchain().getImageExtent().height,
                1
            );

            return getDevice().get().createFramebuffer(info);
        }
    )
{
    if constexpr (vkb::enableVerboseLogging)
    {
        std::cout << "Framebuffer created for " << getSwapchain().getFrameCount() << " frames.\n";
    }
}


Framebuffer::~Framebuffer() noexcept
{
    framebuffers.foreach(
        [](vk::Framebuffer& fb) {
            getDevice().get().destroyFramebuffer(fb);
        }
    );
    colorImageViews.foreach(
        [](vk::ImageView& view) {
            getDevice().get().destroyImageView(view);
        }
    );
}


auto Framebuffer::get() const noexcept -> const vk::Framebuffer&
{
    return framebuffers.get();
}


auto Framebuffer::getAt(uint32_t index) const noexcept -> const vk::Framebuffer&
{
    return framebuffers.getAt(index);
}
