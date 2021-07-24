#pragma once

#include <vkb/FrameSpecificObject.h>
#include <vkb/Buffer.h>
#include <vkb/Image.h>

#include "RenderPass.h"
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
     *  - binding 0: input attachment
     *  - binding 1: input attachment
     *  - binding 2: input attachment
     *  - binding 3: input attachment
     *
     *  - binding 4: storage image
     *  - binding 5: storage buffer
     *  - binding 6: storage buffer
     */
    class DeferredRenderPassDescriptor
    {
    public:
        DeferredRenderPassDescriptor(const vkb::Device& device,
                                     const vkb::Swapchain& swapchain,
                                     const RenderPassDeferred& deferredPass,
                                     ui32 maxTransparentFragsPerPixel);

        void resetValues(vk::CommandBuffer cmdBuf) const;

        auto getProvider() const noexcept -> const DescriptorProviderInterface&;

    private:
        void createFragmentList(const vkb::Device& device,
                                const vkb::Swapchain& swapchain,
                                ui32 maxFragsPerPixel);
        void createDescriptors(const vkb::Device& device, const RenderPassDeferred& renderPass);

        const ui32 ATOMIC_BUFFER_SECTION_SIZE;
        const ui32 FRAG_LIST_BUFFER_SIZE;

        vkb::FrameSpecificObject<vkb::Image> fragmentListHeadPointerImage;
        vkb::FrameSpecificObject<vk::UniqueImageView> fragmentListHeadPointerImageView;
        vkb::FrameSpecificObject<vkb::Buffer> fragmentListBuffer;

        vk::UniqueDescriptorPool descPool;
        vk::UniqueDescriptorSetLayout descLayout;
        vkb::FrameSpecificObject<vk::UniqueDescriptorSet> descSets;
        FrameSpecificDescriptorProvider provider;
    };

    /**
     * @brief The main deferred renderpass
     */
    class RenderPassDeferred : public RenderPass
    {
    public:
        static constexpr ui32 NUM_SUBPASSES = 3;

        RenderPassDeferred(const vkb::Device& device,
                           const vkb::Swapchain& swapchain,
                           ui32 maxTransparentFragsPerPixel);

        void begin(vk::CommandBuffer cmdBuf, vk::SubpassContents subpassContents) override;
        void end(vk::CommandBuffer cmdBuf) override;

        auto getGBuffer() -> vkb::FrameSpecificObject<GBuffer>&;
        auto getGBuffer() const -> const vkb::FrameSpecificObject<GBuffer>&;

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
        static auto makeVkRenderPass(const vkb::Device& device, const vkb::Swapchain& swapchain)
            -> vk::UniqueRenderPass;

    private:
        static inline float mouseDepthValue{ 0.0f };

        void copyMouseDataToBuffers(vk::CommandBuffer cmdBuf);
        vkb::Buffer depthPixelReadBuffer;
        float* depthBufMap = reinterpret_cast<float*>(depthPixelReadBuffer.map());

        const vkb::Swapchain& swapchain;

        vk::Extent2D framebufferSize;
        vkb::FrameSpecificObject<GBuffer> gBuffers;
        vkb::FrameSpecificObject<Framebuffer> framebuffers;

        std::array<vk::ClearValue, 6> clearValues;

        // Descriptor
        DeferredRenderPassDescriptor descriptor;
    };
} // namespace trc
