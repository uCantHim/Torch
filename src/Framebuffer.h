#pragma once

#include "vkb/Image.h"
#include "vkb/FrameSpecificObject.h"

/**
 * @brief A standard framebuffer
 *
 * Attachment 0: Color attachment. Creates views from the swapchain's
 *               images.
 * Attachment 1: 32-bit depth attachment.
 */
class Framebuffer : public vkb::VulkanBase
{
public:
    explicit Framebuffer(const vk::RenderPass& renderpass);
    Framebuffer(const Framebuffer&) = delete;
    Framebuffer(Framebuffer&&) noexcept = default;
    ~Framebuffer() noexcept;

    Framebuffer& operator=(const Framebuffer&) = delete;
    Framebuffer& operator=(Framebuffer&&) noexcept = default;

    auto get() const noexcept -> vk::Framebuffer;
    auto getAt(uint32_t index) const noexcept -> vk::Framebuffer;

private:
    vkb::FrameSpecificObject<vk::ImageView> colorImageViews;
    vkb::FrameSpecificObject<vkb::Image> depthImages;
    vkb::FrameSpecificObject<vk::UniqueImageView> depthImageViews;

    vkb::FrameSpecificObject<vk::Framebuffer> framebuffers;
};
