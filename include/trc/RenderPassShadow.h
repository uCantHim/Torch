#pragma once

#include "trc/Framebuffer.h"
#include "trc/Types.h"
#include "trc/base/Image.h"
#include "trc/core/RenderPass.h"

namespace trc
{
    struct ShadowPassCreateInfo
    {
        ui32 shadowIndex;
        uvec2 resolution;
    };

    /**
     * @brief A pass that renders a shadow map
     *
     * Only renders a single shadow map. Point lights thus have multiple
     * shadow passes.
     */
    class RenderPassShadow : public RenderPass
    {
    public:
        /**
         * @param const Device& device
         * @param const ShadowPassCreateInfo& info
         */
        RenderPassShadow(const Device& device,
                         const ShadowPassCreateInfo& info,
                         const DeviceMemoryAllocator& alloc = DefaultDeviceMemoryAllocator{});

        /**
         * Updates the shadow matrix in the descriptor and starts the
         * renderpass.
         */
        void begin(vk::CommandBuffer cmdBuf,
                   vk::SubpassContents subpassContents,
                   FrameRenderState&) override;
        void end(vk::CommandBuffer cmdBuf) override;

        auto getResolution() const noexcept -> uvec2;

        /**
         * @return ui32 The shadow pass's index into the shadow matrix
         *              buffer. Vertex shaders in the shadow stage use this
         *              to get their view- projection matrix.
         */
        auto getShadowMatrixIndex() const noexcept -> ui32;

        auto getShadowImage() const -> const Image&;
        auto getShadowImageView() const -> vk::ImageView;

        static auto makeVkRenderPass(const Device& device) -> vk::UniqueRenderPass;

    private:
        const uvec2 resolution;
        const ui32 shadowMatrixIndex;

        Image depthImage;
        Framebuffer framebuffer;
    };
} // namespace trc
