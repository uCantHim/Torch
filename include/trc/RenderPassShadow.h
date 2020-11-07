#pragma once

#include <vkb/Image.h>

#include "Types.h"
#include "RenderStage.h"
#include "RenderPass.h"

namespace trc
{
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
         * @param uvec2 resolution Resolution of the shadow map
         */
        explicit RenderPassShadow(uvec2 resolution);

        /**
         * Updates the shadow matrix in the descriptor and starts the
         * renderpass.
         */
        void begin(vk::CommandBuffer cmdBuf, vk::SubpassContents subpassContents) override;
        void end(vk::CommandBuffer cmdBuf) override;

        auto getResolution() const noexcept -> uvec2;

        /**
         * @return ui32 The shadow pass's index into the shadow matrix
         *              buffer. Vertex shaders in the shadow stage use this
         *              to get their view- projection matrix.
         */
        auto getShadowMatrixIndex() const noexcept -> ui32;

        /**
         * This is called by the light registry when shadow passes are
         * re-ordered. Please don't call this manually.
         */
        void setShadowMatrixIndex(ui32 newIndex) noexcept;

        auto getDepthImage(ui32 imageIndex) const -> const vkb::Image&;
        auto getDepthImageView(ui32 imageIndex) const -> vk::ImageView;

    private:
        const uvec2 resolution;
        ui32 shadowMatrixIndex;

        vkb::FrameSpecificObject<vkb::Image> depthImages;
        vkb::FrameSpecificObject<vk::UniqueImageView> depthImageViews;
        vkb::FrameSpecificObject<vk::UniqueFramebuffer> framebuffers;
    };
} // namespace trc
