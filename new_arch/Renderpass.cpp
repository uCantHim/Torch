#include "Renderpass.h"

#include <numeric>

#include <vkb/VulkanBase.h>



RenderPass::RenderPass(
    const vk::RenderPassCreateInfo& createInfo,
    std::vector<vk::ClearValue> clearValues)
    :
    renderPass(vkb::VulkanBase::getDevice()->createRenderPassUnique(createInfo)),
    subPasses(createInfo.subpassCount),
    clearValues(std::move(clearValues))
{
    std::iota(subPasses.begin(), subPasses.end(), 0);
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

auto RenderPass::getNumSubPasses() const noexcept -> uint32_t
{
    return subPasses.size();
}

auto RenderPass::getSubPasses() const noexcept -> const std::vector<SubPass::ID>&
{
    return subPasses;
}

auto RenderPass::getClearValues() const noexcept -> const std::vector<vk::ClearValue>&
{
    return clearValues;
}



auto makeDefaultSwapchainColorAttachment(vkb::Swapchain& swapchain) -> vk::AttachmentDescription
{
    return vk::AttachmentDescription(
        vk::AttachmentDescriptionFlags(),
        swapchain.getImageFormat(),
        vk::SampleCountFlagBits::e1,
        vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore, // load/store ops
        vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare, // stencil ops
        vk::ImageLayout::eUndefined, vk::ImageLayout::ePresentSrcKHR
    );
}

auto makeDefaultDepthStencilAttachment() -> vk::AttachmentDescription
{
    return vk::AttachmentDescription(
        vk::AttachmentDescriptionFlags(),
        vk::Format::eD24UnormS8Uint,
        vk::SampleCountFlagBits::e1,
        vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eDontCare, // load/store ops
        vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eDontCare, // stencil ops
        vk::ImageLayout::eUndefined, vk::ImageLayout::eDepthStencilAttachmentOptimal
    );
}
