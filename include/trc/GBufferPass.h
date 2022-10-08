#pragma once

#include "trc/base/FrameSpecificObject.h"

#include "trc/core/RenderPass.h"
#include "trc/Framebuffer.h"
#include "trc/GBuffer.h"

namespace trc
{
    /**
     * @brief The main deferred renderpass
     *
     * After the renderpass has completed, the g-buffer images are in the
     * following layouts:
     *  - normals:   eGeneral
     *  - albedo:    eGeneral
     *  - materials: eGeneral
     *  - depth:     eShaderReadOnlyOptimal
     */
    class GBufferPass : public RenderPass
    {
    public:
        static constexpr ui32 NUM_SUBPASSES = 2;

        struct SubPasses
        {
            static constexpr SubPass::ID gBuffer{ 0 };
            static constexpr SubPass::ID transparency{ 1 };
        };

        GBufferPass(const Device& device,
                    FrameSpecific<GBuffer>& gBuffer);

        void begin(vk::CommandBuffer cmdBuf,
                   vk::SubpassContents subpassContents,
                   FrameRenderState&) override;
        void end(vk::CommandBuffer cmdBuf) override;

        void setClearColor(vec4 color);

        /**
         * @brief Make only a vk::RenderPass but don't allocate any
         *        resources to back it up
         *
         * Can be used to make dummy render passes, for example during
         * pipeline creation.
         *
         * @param const Swapchain& swapchain Used to determine the
         *        image format of the color attachment.
         */
        static auto makeVkRenderPass(const Device& device) -> vk::UniqueRenderPass;

    private:
        FrameSpecific<GBuffer>& gBuffer;
        uvec2 framebufferSize;
        FrameSpecific<Framebuffer> framebuffers;

        std::array<vk::ClearValue, 4> clearValues;
    };
} // namespace trc
