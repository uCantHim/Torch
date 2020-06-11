#pragma once

#include <vulkan/vulkan.hpp>

#include "data_utils/SelfManagedObject.h"
#include "Framebuffer.h"

// Will probably not be used but I can define a consistent ID type this way
struct SubPass
{
    using ID = SelfManagedObject<SubPass>::ID;
};

/**
 * @brief A renderpass
 *
 * Contains subpasses.
 */
class RenderPass : public SelfManagedObject<RenderPass>
{
public:
    RenderPass();
    explicit RenderPass(const vk::RenderPassCreateInfo& createInfo);

    auto operator*() noexcept -> vk::RenderPass;
    auto operator*() const noexcept -> vk::RenderPass;
    auto get() const noexcept -> vk::RenderPass;

    auto getSubPasses() const noexcept -> const std::vector<SubPass::ID>&;
    auto getFramebuffer() const noexcept -> vk::Framebuffer;

private:
    vk::UniqueRenderPass renderPass;
    Framebuffer framebuffer;

    // Just a list of contiguous numbers. Could be a simple int but I like
    // the consistency of having range-based for loops everywhere.
    std::vector<SubPass::ID> subpasses;
};
