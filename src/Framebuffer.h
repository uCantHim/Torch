#pragma once
#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

#include "vkb/FrameSpecificObject.h"

/*
A standard framebuffer.
Has one color attachment, which is initialized as one image in
the swapchain. */
class Framebuffer : public vkb::VulkanBase
{
public:
    explicit Framebuffer(const vk::RenderPass& renderpass);
    Framebuffer(const Framebuffer&) = delete;
    Framebuffer(Framebuffer&&) noexcept = default;
    ~Framebuffer() noexcept;

    Framebuffer& operator=(const Framebuffer&) = delete;
    Framebuffer& operator=(Framebuffer&&) noexcept = default;

    [[nodiscard]]
    auto get() const noexcept -> const vk::Framebuffer&;
    [[nodiscard]]
    auto getAt(uint32_t index) const noexcept -> const vk::Framebuffer&;

private:
    vkb::FrameSpecificObject<vk::ImageView> colorImageViews;
    vkb::FrameSpecificObject<vk::Framebuffer> framebuffers;
};



#endif
