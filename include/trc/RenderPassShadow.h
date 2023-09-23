#pragma once

#include "trc/base/Image.h"
#include "trc/base/FrameSpecificObject.h"

#include "trc/Types.h"
#include "trc/core/RenderPass.h"
#include "trc/Framebuffer.h"

namespace trc
{
    class Window;

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
         * @param const FrameClock& clock A frame clock for the shadow images.
         *                                Is usually the same clock used for
         *                                the rest of the render pipeline.
         * @param const ShadowPassCreateInfo& info
         */
        RenderPassShadow(const Device& device,
                         const FrameClock& clock,
                         const ShadowPassCreateInfo& info);

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

        auto getShadowImage(ui32 frameIndex) const -> const Image&;
        auto getShadowImageView(ui32 frameIndex) const -> vk::ImageView;

    private:
        const uvec2 resolution;
        ui32 shadowMatrixIndex;

        FrameSpecific<Image> depthImages;
        FrameSpecific<Framebuffer> framebuffers;
    };
} // namespace trc
