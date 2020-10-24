#pragma once

#include <vkb/FrameSpecificObject.h>
#include <vkb/Buffer.h>
#include <vkb/Image.h>

#include "RenderStage.h"
#include "RenderPass.h"
#include "utils/Camera.h"

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
        DeferredRenderPassDescriptor(const RenderPassDeferred& deferredPass,
                                     const vkb::Swapchain& swapchain,
                                     ui32 maxTransparentFragsPerPixel);

        void resetValues(vk::CommandBuffer cmdBuf) const;

        auto getProvider() const noexcept -> const DescriptorProviderInterface&;

    private:
        void createFragmentList(const vkb::Swapchain& swapchain, ui32 maxFragsPerPixel);
        void createDescriptors(const RenderPassDeferred& renderPass);

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

        RenderPassDeferred(const vkb::Swapchain& swapchain, ui32 maxTransparentFragsPerPixel);

        void begin(vk::CommandBuffer cmdBuf, vk::SubpassContents subpassContents) override;
        void end(vk::CommandBuffer cmdBuf) override;

        auto getAttachmentImageViews(ui32 imageIndex) const noexcept
           -> const std::vector<vk::UniqueImageView>&;

        /**
         * @return The descriptor for the deferred renderpass
         */
        auto getDescriptor() const noexcept -> const DeferredRenderPassDescriptor&;

        /**
         * A shortcut for getDescriptor().getProvider()
         */
        auto getDescriptorProvider() const noexcept -> const DescriptorProviderInterface&;

        /**
         * @return float Depth of the pixel which contains the mouse cursor.
         */
        static auto getMouseDepthValue() -> float;

    private:
        static inline float mouseDepthValue{ 0.0f };

        void copyMouseDepthValueToBuffer(vk::CommandBuffer cmdBuf);
        void readMouseDepthValueFromBuffer();
        vkb::Buffer depthPixelReadBuffer;

        const vkb::Swapchain& swapchain;

        // Attachments
        std::vector<std::vector<vkb::Image>> attachmentImages;
        std::vector<std::vector<vk::UniqueImageView>> attachmentImageViews;

        vk::Extent2D framebufferSize;
        vkb::FrameSpecificObject<vk::UniqueFramebuffer> framebuffers;

        std::array<vk::ClearValue, 6> clearValues;

        // Descriptor
        DeferredRenderPassDescriptor descriptor;
    };

    /**
     * This function does not have any calculational overhead. It is a more
     * intuitive alternative to RenderPassDeferred::getMouseDepthValue.
     *
     * @return float Depth of the pixel which contains the mouse cursor.
     *               Is zero if no depth value has been read. Is the last
     *               read depth value if the cursor is not in a window.
     */
    extern auto getMouseDepth() -> float;

    /**
     * This function calculates the mouse position when it's called. It is
     * a good idea to call this only once per frame and re-use the result.
     *
     * @param const Camera& camera Reconstructing the mouse coursor's world
     *                             position requires the view- and projection
     *                             matrices that were used for the scene that
     *                             the cursor is in.
     *
     * @return vec3 Position of the mouse cursor in the world.
     */
    extern auto getMouseWorldPos(const Camera& camera) -> vec3;

    /**
     * Calculates the mouse cursor position in the world at a specific
     * depth.
     *
     * @param const Camera& camera Reconstructing the mouse coursor's world
     *                             position requires the view- and projection
     *                             matrices that were used for the scene that
     *                             the cursor is in.
     * @param float         depth  A depth value in the range [0, 1].
     *
     * @return vec3 Position of the mouse cursor in the world.
     */
    extern auto getMouseWorldPosAtDepth(const Camera& camera, float depth) -> vec3;
} // namespace trc
