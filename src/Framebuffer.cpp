#include "Framebuffer.h"
#include <vulkan/vulkan.hpp>



trc::Framebuffer::Framebuffer(
    const vkb::Device& device,
    vk::RenderPass renderPass,
    const uvec2 size,
    std::vector<vk::UniqueImageView> attachments,
    std::vector<vk::ImageView> additionalAttachments)
    :
    attachmentImageViews(std::move(attachments)),
    additionalAttachmentImageViews(std::move(additionalAttachments)),
    size(size)
{
    std::vector<vk::ImageView> tmpAttachmentViews;
    for (const auto& view : attachmentImageViews) {
        tmpAttachmentViews.push_back(*view);
    }
    for (auto view : additionalAttachmentImageViews) {
        tmpAttachmentViews.push_back(view);
    }

    // Create the framebuffer
    framebuffer = device->createFramebufferUnique(
        vk::FramebufferCreateInfo(
            {},
            renderPass,
            tmpAttachmentViews,
            size.x, size.y,
            1 // layers
        )
    );
}

trc::Framebuffer::Framebuffer(
    const vkb::Device& device,
    vk::RenderPass renderPass,
    const uvec2 size,
    vk::FramebufferAttachmentsCreateInfo attachmentInfo)
    :
    size(size)
{
    vk::StructureChain chain{
        vk::FramebufferCreateInfo(
            vk::FramebufferCreateFlagBits::eImageless,
            renderPass,
            0, nullptr, // attachments
            size.x, size.y,
            1 // layers
        ),
        attachmentInfo
    };

    framebuffer = device->createFramebufferUnique(chain.get<vk::FramebufferCreateInfo>());
}

auto trc::Framebuffer::operator*() const -> vk::Framebuffer
{
    return *framebuffer;
}

auto trc::Framebuffer::getSize() const -> vec2
{
    return size;
}

auto trc::Framebuffer::getAttachmentView(ui32 attachmentIndex) const -> vk::ImageView
{
    if (attachmentIndex < attachmentImageViews.size()) {
        return *attachmentImageViews.at(attachmentIndex);
    }
    else {
        return additionalAttachmentImageViews.at(attachmentIndex - attachmentImageViews.size());
    }
}
