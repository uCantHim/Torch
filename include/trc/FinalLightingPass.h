#pragma once

#include <vkb/FrameSpecificObject.h>

#include "core/RenderPass.h"
#include "core/Pipeline.h"
#include "GBuffer.h"

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

        FinalLightingPass(const vkb::Device& device,
                          const RenderTarget& target,
                          uvec2 offset,
                          uvec2 size,
                          TorchRenderConfig& config);

        void begin(vk::CommandBuffer cmdBuf, vk::SubpassContents, FrameRenderState&) override;
        void end(vk::CommandBuffer) override {}

        void setTargetArea(uvec2 offset, uvec2 size);
        void setRenderTarget(const vkb::Device& device, const RenderTarget& target);

    private:
        void createDescriptors(const vkb::Device& device, const vkb::FrameClock& frameClock);
        void updateDescriptors(const vkb::Device& device, const RenderTarget& target);

        const RenderTarget* renderTarget;
        const TorchRenderConfig* renderConfig;

        vk::UniqueDescriptorPool descPool;
        vk::UniqueDescriptorSetLayout descLayout;
        vkb::FrameSpecific<vk::UniqueDescriptorSet> descSets;
        FrameSpecificDescriptorProvider provider;

        PipelineLayout layout;
        Pipeline pipeline;

        vec2 renderOffset;
        vec2 renderSize;
        uvec3 groupCount;
    };
} // namespace trc
