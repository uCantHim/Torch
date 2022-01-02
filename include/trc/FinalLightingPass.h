#pragma once

#include <vkb/FrameSpecificObject.h>

#include "core/RenderPass.h"
#include "core/Pipeline.h"
#include "GBuffer.h"

namespace trc
{
    class Window;
    class DeferredRenderConfig;

    /**
     * @brief
     */
    class FinalLightingPass : public RenderPass
    {
    public:
        FinalLightingPass(const Window& window, DeferredRenderConfig& config);

        FinalLightingPass(FinalLightingPass&&) noexcept = default;
        ~FinalLightingPass() noexcept = default;
        auto operator=(FinalLightingPass&&) noexcept -> FinalLightingPass& = default;

        void begin(vk::CommandBuffer cmdBuf, vk::SubpassContents) override;
        void end(vk::CommandBuffer) override {}

        void resize(uvec2 windowSize);

    private:
        const Window* window;

        PipelineLayout layout;
        Pipeline pipeline;
        uvec3 groupCount;
    };
} // namespace trc
