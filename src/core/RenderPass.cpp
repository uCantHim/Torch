#include "trc/core/RenderPass.h"

#include "trc/base/Swapchain.h"



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
