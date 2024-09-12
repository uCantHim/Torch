#include "trc/Framebuffer.h"

#include "trc/base/Device.h"



trc::Framebuffer::Framebuffer(
    const Device& device,
    vk::RenderPass renderPass,
    const uvec2 size,
    const vk::ArrayProxy<vk::ImageView>& attachments)
    :
    attachmentImageViews(attachments.begin(), attachments.end())
{
    // Create the framebuffer
    framebuffer = device->createFramebufferUnique(
        vk::FramebufferCreateInfo(
            {},
            renderPass,
            attachments,
            size.x, size.y,
            1 // layers
        )
    );
}

trc::Framebuffer::Framebuffer(
    const Device& device,
    vk::RenderPass renderPass,
    const uvec2 size,
    vk::FramebufferAttachmentsCreateInfo attachmentInfo)
{
    vk::StructureChain chain{
        vk::FramebufferCreateInfo(
            vk::FramebufferCreateFlagBits::eImageless,
            renderPass,
            attachmentInfo.attachmentImageInfoCount, nullptr, // attachments
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

auto trc::Framebuffer::getAttachmentView(ui32 attachmentIndex) const -> vk::ImageView
{
    return attachmentImageViews.at(attachmentIndex);
}
