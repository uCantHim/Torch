#pragma once

#include <vkb/FrameSpecificObject.h>
#include <vkb/Buffer.h>
#include <vkb/Image.h>

#include "core/RenderPass.h"
#include "core/DescriptorProvider.h"
#include "Framebuffer.h"
#include "Camera.h"
#include "GBuffer.h"

namespace trc
{
    class RenderPassDeferred;

    /**
     * @brief Resources and descriptor set for deferred renderpasses
     *
     * Provides:
     *  - binding 0: storage image rgba16f  (normals)
     *  - binding 1: storage image r32ui    (albedo)
     *  - binding 2: storage image r32ui    (materials)
     *  - binding 3: combined image sampler (depth)
     *
     *  - binding 4: storage image  (head pointer image)
     *  - binding 5: storage buffer (allocator)
     *  - binding 6: storage buffer (fragment list)
     *
     *  - binding 7: storage image rgba8 (swapchain image)
     */
    class DeferredRenderPassDescriptor
    {
    public:
        DeferredRenderPassDescriptor(const vkb::Device& device,
                                     const vkb::Swapchain& swapchain,
                                     const vkb::FrameSpecific<GBuffer>& gBuffer);

        auto getProvider() const noexcept -> const DescriptorProviderInterface&;

    private:
        vk::UniqueDescriptorPool descPool;
        vk::UniqueDescriptorSetLayout descLayout;
        vkb::FrameSpecific<vk::UniqueDescriptorSet> descSets;
        FrameSpecificDescriptorProvider provider;
    };

    /**
     * @brief The main deferred renderpass
     *
     * After the renderpass has completed, the g-buffer images are in the
     * following layouts:
     *  - normals:   eGeneral
     *  - albedo:    eGeneral
     *  - materials: eGeneral
     *  - depth:     eShaderReadOnlyOptimal
     *  - swapchain: ePresentSrcKHR
     */
    class RenderPassDeferred : public RenderPass
    {
    public:
        static constexpr ui32 NUM_SUBPASSES = 2;

        struct SubPasses
        {
            static constexpr SubPass::ID gBuffer{ 0 };
            static constexpr SubPass::ID transparency{ 1 };
        };

        RenderPassDeferred(const vkb::Device& device,
                           const vkb::Swapchain& swapchain,
                           vkb::FrameSpecific<GBuffer>& gBuffer);

        void begin(vk::CommandBuffer cmdBuf, vk::SubpassContents subpassContents) override;
        void end(vk::CommandBuffer cmdBuf) override;

        /**
         * @return The descriptor for the deferred renderpass
         */
        auto getDescriptor() const noexcept -> const DeferredRenderPassDescriptor&;

        /**
         * A shortcut for getDescriptor().getProvider()
         */
        auto getDescriptorProvider() const noexcept -> const DescriptorProviderInterface&;

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
        static inline float mouseDepthValue{ 0.0f };

        void copyMouseDataToBuffers(vk::CommandBuffer cmdBuf);
        vkb::Buffer depthPixelReadBuffer;
        ui32* depthBufMap = reinterpret_cast<ui32*>(depthPixelReadBuffer.map());

        const vkb::Swapchain& swapchain;

        vkb::FrameSpecific<GBuffer>& gBuffer;
        uvec2 framebufferSize;
        vkb::FrameSpecific<Framebuffer> framebuffers;

        std::array<vk::ClearValue, 6> clearValues;

        // Descriptor
        DeferredRenderPassDescriptor descriptor;
    };
} // namespace trc
