#pragma once

#include "../Types.h"

namespace vkb {
    class Swapchain;
}

namespace trc
{
    // Will probably not be used but I can define a consistent ID type this way
    struct SubPass
    {
        using ID = TypesafeID<SubPass>;
    };

    /**
     * @brief A renderpass
     *
     * Contains subpasses.
     */
    class RenderPass
    {
    public:
        /**
         * @brief Constructor
         */
        RenderPass(vk::UniqueRenderPass renderPass, ui32 subpassCount);

        RenderPass(RenderPass&&) noexcept = default;
        virtual ~RenderPass() = default;
        auto operator=(RenderPass&&) noexcept -> RenderPass& = default;

        auto operator*() const noexcept -> vk::RenderPass;
        auto get() const noexcept -> vk::RenderPass;

        auto getNumSubPasses() const noexcept -> ui32;

        virtual void begin(vk::CommandBuffer cmdBuf, vk::SubpassContents subpassContents) = 0;
        virtual void end(vk::CommandBuffer cmdBuf) = 0;

    protected:
        vk::UniqueRenderPass renderPass;
        ui32 numSubpasses;
    };


    /**
     * @brief Create a default color attachment description
     *
     * Use this for a quick color attachment.
     *
     * The attachment is in the format of the swapchain's images. It's cleared
     * on load and stored on store. The sample count is 1. The image is
     * expected in an undefined layout and will be transformed into presentable
     * layout.
     */
    auto makeDefaultSwapchainColorAttachment(const vkb::Swapchain& swapchain)
        -> vk::AttachmentDescription;

    /**
     * @brief Create a default depth attachment description
     *
     * Use this for a quick depth attachment with sensible defaults.
     *
     * 24 bit depth, 8 bit stencil format. 1 sample. Cleared on load, don't
     * care on store. The image is expected in an undefined layout and will
     * be transformed into a depthStencilAttachmentOptimal layout.
     */
    auto makeDefaultDepthStencilAttachment() -> vk::AttachmentDescription;
} // namespace trc
