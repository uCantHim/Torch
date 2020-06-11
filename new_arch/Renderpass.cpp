#include "Renderpass.h"



RenderPass::RenderPass()
{
}

RenderPass::RenderPass(const vk::RenderPassCreateInfo& createInfo)
    :
    renderPass(vkb::VulkanBase::getDevice()->createRenderPassUnique(createInfo)),
    framebuffer(*renderPass)
{
    subpasses = { 0 };
}

auto RenderPass::operator*() noexcept -> vk::RenderPass
{
    return *renderPass;
}

auto RenderPass::operator*() const noexcept -> vk::RenderPass
{
    return *renderPass;
}

auto RenderPass::get() const noexcept -> vk::RenderPass
{
    return *renderPass;
}

auto RenderPass::getSubPasses() const noexcept -> const std::vector<SubPass::ID>&
{
    return subpasses;
}

auto RenderPass::getFramebuffer() const noexcept -> vk::Framebuffer
{
    return framebuffer.get();
}
