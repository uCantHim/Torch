#pragma once

#include "trc/base/FrameSpecificObject.h"

#include "trc/core/RenderPass.h"
#include "trc/core/Pipeline.h"
#include "trc/GBuffer.h"

namespace trc
{
    class RenderTarget;
    class TorchRenderConfig;

    /**
     * @brief
     */
    class FinalLightingPass : public RenderPass
    {
    public:
        FinalLightingPass(FinalLightingPass&&) noexcept = default;
        auto operator=(FinalLightingPass&&) noexcept -> FinalLightingPass& = default;
        ~FinalLightingPass() noexcept = default;

        FinalLightingPass(const Device& device,
                          const RenderTarget& target,
                          uvec2 offset,
                          uvec2 size,
                          TorchRenderConfig& config);

        void begin(vk::CommandBuffer cmdBuf, vk::SubpassContents, FrameRenderState&) override;
        void end(vk::CommandBuffer) override {}

        void setTargetArea(ivec2 offset, uvec2 size);
        void setRenderTarget(const Device& device, const RenderTarget& target);

    private:
        void createDescriptors(const Device& device, const FrameClock& frameClock);
        void updateDescriptors(const Device& device, const RenderTarget& target);

        const RenderTarget* renderTarget;
        const TorchRenderConfig* renderConfig;

        vk::UniqueDescriptorPool descPool;
        vk::UniqueDescriptorSetLayout descLayout;
        FrameSpecific<vk::UniqueDescriptorSet> descSets;
        FrameSpecificDescriptorProvider provider;

        PipelineLayout layout;
        Pipeline pipeline;

        vec2 renderOffset;
        vec2 renderSize;
        uvec3 groupCount;
    };
} // namespace trc
