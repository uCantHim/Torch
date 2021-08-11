#pragma once

#include <vkb/basics/Swapchain.h>
#include <vkb/Image.h>

#include "Types.h"

namespace trc
{
    class Framebuffer
    {
    public:
        /**
         * @brief Create a framebuffer
         *
         * The combined number of attachments in `attachments` and
         * `additionalAttachments` must be > 0. If you want to create an
         * imageless framebuffer, use a constructor overload that accepts
         * a vk::FramebufferAttachmentsCreateInfo struct.
         *
         * @param const vkb::Device& device
         * @param vk::RenderPass renderPass The type of render pass that
         *        the framebuffer will be compatible with.
         * @param uvec2 size Size of the framebuffer in pixels
         * @param std::vector<vk::UniqueImageView> attachments Attachment
         *        images. Images get attached to the framebuffer in the
         *        same order. May be empty if `additionalAttachments` is
         *        non-empty.
         * @param std::vector<vk::ImageView> additionalAttachments
         *        Additional image views for which the images are stored
         *        elsewhere. This can be used for view on swapchain images.
         *        Attachments in this array will occupy indices after the
         *        ones from the `attachments` parameter. May be empty if
         *        `attachments` is non-empty.
         */
        Framebuffer(const vkb::Device& device,
                    vk::RenderPass renderPass,
                    uvec2 size,
                    std::vector<vk::UniqueImageView> attachments = {},
                    std::vector<vk::ImageView> additionalAttachments = {});

        /**
         * @brief Create an imageless framebuffer
         *
         * @param const vkb::Device& device
         * @param vk::RenderPass renderPass The type of render pass that
         *        the framebuffer will be compatible with.
         * @param uvec2 size Size of the framebuffer in pixels
         */
        Framebuffer(const vkb::Device& device,
                    vk::RenderPass renderPass,
                    uvec2 size,
                    vk::FramebufferAttachmentsCreateInfo attachmentInfo);

        Framebuffer(Framebuffer&&) = default;
        Framebuffer& operator=(Framebuffer&&) = default;
        ~Framebuffer() = default;

        Framebuffer(const Framebuffer&) = delete;
        Framebuffer& operator=(const Framebuffer&) = delete;

        auto operator*() const -> vk::Framebuffer;

        auto getAttachmentView(ui32 attachmentIndex) const -> vk::ImageView;

    private:
        std::vector<vk::UniqueImageView> attachmentImageViews;
        std::vector<vk::ImageView> additionalAttachmentImageViews;
        vk::UniqueFramebuffer framebuffer;
    };
} // namespace trc
