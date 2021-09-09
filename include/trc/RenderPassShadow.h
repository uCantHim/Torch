#pragma once

#include <vkb/Image.h>
#include <vkb/FrameSpecificObject.h>

#include "Types.h"
#include "core/RenderPass.h"
#include "Framebuffer.h"

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
         * @param const Window& window
         * @param const ShadowPassCreateInfo& info
         */
        RenderPassShadow(const Window& window, const ShadowPassCreateInfo& info);

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

        auto getShadowImage(ui32 frameIndex) const -> const vkb::Image&;
        auto getShadowImageView(ui32 frameIndex) const -> vk::ImageView;

    private:
        const uvec2 resolution;
        ui32 shadowMatrixIndex;

        vkb::FrameSpecific<vkb::Image> depthImages;
        vkb::FrameSpecific<Framebuffer> framebuffers;
    };
} // namespace trc
