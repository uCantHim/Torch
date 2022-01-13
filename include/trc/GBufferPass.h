#pragma once

#include <vkb/FrameSpecificObject.h>

#include "core/RenderPass.h"
#include "Framebuffer.h"
#include "GBuffer.h"

namespace trc
{
    class Camera;

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

        GBufferPass(const vkb::Device& device,
                           const vkb::Swapchain& swapchain,
                           vkb::FrameSpecific<GBuffer>& gBuffer);

        void begin(vk::CommandBuffer cmdBuf, vk::SubpassContents subpassContents) override;
        void end(vk::CommandBuffer cmdBuf) override;

        void setClearColor(vec4 color);

        /**
         * @brief Get the depth of pixel under the mouse cursor
         *
         * @return float Depth of the pixel which contains the mouse cursor.
         *               Is the last read depth value if the cursor is not
         *               in a window.
         */
        auto getMouseDepth() const noexcept -> float;

        /**
         * @brief Get world position of mouse cursor
         *
         * @param const Camera& camera The camera that was used to render
         *        with this render pass. Required for projection and view
         *        matrices.
         *
         * @return vec3
         */
        auto getMousePos(const Camera& camera) const noexcept -> vec3;

        /**
         * @brief Make only a vk::RenderPass but don't allocate any
         *        resources to back it up
         *
         * Can be used to make dummy render passes, for example during
         * pipeline creation.
         *
         * @param const vkb::Swapchain& swapchain Used to determine the
         *        image format of the color attachment.
         */
        static auto makeVkRenderPass(const vkb::Device& device) -> vk::UniqueRenderPass;

    private:
        void copyMouseDataToBuffers(vk::CommandBuffer cmdBuf);
        vkb::Buffer depthPixelReadBuffer;
        ui32* depthBufMap{ depthPixelReadBuffer.map<ui32*>() };

        const vkb::Swapchain& swapchain;
        vkb::FrameSpecific<GBuffer>& gBuffer;

        uvec2 framebufferSize;
        vkb::FrameSpecific<Framebuffer> framebuffers;

        std::array<vk::ClearValue, 4> clearValues;
    };
} // namespace trc
