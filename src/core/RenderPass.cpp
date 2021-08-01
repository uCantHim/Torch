#include "RenderPass.h"

#include <vkb/basics/Swapchain.h>



trc::RenderPass::RenderPass(vk::UniqueRenderPass renderPass, ui32 subpassCount)
    :
    renderPass(std::move(renderPass)),
    numSubpasses(subpassCount)
{
}

auto trc::RenderPass::operator*() const noexcept -> vk::RenderPass
{
    return *renderPass;
}

auto trc::RenderPass::get() const noexcept -> vk::RenderPass
{
    return *renderPass;
}

auto trc::RenderPass::getNumSubPasses() const noexcept -> ui32
{
    return numSubpasses;
}



auto trc::makeDefaultSwapchainColorAttachment(const vkb::Swapchain& swapchain)
    -> vk::AttachmentDescription
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

auto trc::makeDefaultDepthStencilAttachment() -> vk::AttachmentDescription
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
