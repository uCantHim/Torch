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
    depthImages(
        [](uint32_t) -> vkb::Image
        {
            return vkb::Image(vk::ImageCreateInfo(
                vk::ImageCreateFlags(),
                vk::ImageType::e2D, vk::Format::eD32Sfloat,
                vk::Extent3D{ getSwapchain().getImageExtent(), 1 },
                1, 1,
                vk::SampleCountFlagBits::e1,
                vk::ImageTiling::eOptimal,
                vk::ImageUsageFlagBits::eDepthStencilAttachment,
                vk::SharingMode::eExclusive
            ));
        }
    ),
    depthImageViews(
        [&](uint32_t imageIndex)
        {
            return depthImages.getAt(imageIndex).createView(
                vk::ImageViewType::e2D, vk::Format::eD32Sfloat, {},
                vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eDepth, 0, 1, 0, 1)
            );
        }
    ),
    framebuffers(
        [&](uint32_t imageIndex) -> vk::Framebuffer {
            std::vector<vk::ImageView> attachments = {
                colorImageViews.getAt(imageIndex),
                *depthImageViews.getAt(imageIndex)
            };
            vk::FramebufferCreateInfo info(
                vk::FramebufferCreateFlags(),
                renderpass,
                static_cast<uint32_t>(attachments.size()), attachments.data(),
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


auto Framebuffer::get() const noexcept -> vk::Framebuffer
{
    return framebuffers.get();
}


auto Framebuffer::getAt(uint32_t index) const noexcept -> vk::Framebuffer
{
    return framebuffers.getAt(index);
}
