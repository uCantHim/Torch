#pragma once

#include <vector>

#include <vkb/FrameSpecificObject.h>

#include "trc/core/Pipeline.h"
#include "trc/core/RenderPass.h"
#include "trc/ray_tracing/AccelerationStructure.h"
#include "trc/ray_tracing/FinalCompositingPass.h"
#include "trc/ray_tracing/RaygenDescriptor.h"
#include "trc/ray_tracing/RayBuffer.h"
#include "trc/ray_tracing/RayPipelineBuilder.h"

namespace trc
{
    class RenderTarget;
    class TorchRenderConfig;

    /**
     * @brief A renderpass that does nothing
     */
    class RayTracingPass : public RenderPass
    {
    public:
        RayTracingPass(const RayTracingPass&) = delete;
        RayTracingPass(RayTracingPass&&) noexcept = delete;
        RayTracingPass& operator=(const RayTracingPass&) = delete;
        RayTracingPass& operator=(RayTracingPass&&) noexcept = delete;

        ~RayTracingPass() = default;

        RayTracingPass(const Instance& instance,
                       TorchRenderConfig& renderConfig,
                       const rt::TLAS& tlas,
                       vkb::FrameSpecific<rt::RayBuffer> rayBuffer,
                       const RenderTarget& target);

        void begin(vk::CommandBuffer, vk::SubpassContents, FrameRenderState&) override;
        void end(vk::CommandBuffer) override;

    private:
        struct RayTracingCall
        {
            u_ptr<PipelineLayout> layout;
            u_ptr<Pipeline> pipeline;
            u_ptr<rt::ShaderBindingTable> sbt;

            ui32 raygenTableIndex;
            ui32 missTableIndex;
            ui32 hitTableIndex;
            ui32 callableTableIndex;

            rt::RayBuffer::Image outputImage;
        };

        void addRayCall(RayTracingCall call);
        auto makeDescriptor(rt::RayBuffer::Image image) -> const DescriptorProviderInterface&;

        static constexpr ui32 kMaxReccursionDepth{ 16 };

        const Instance& instance;
        const rt::TLAS& tlas;

        vkb::FrameSpecific<rt::RayBuffer> rayBuffer;

        rt::RaygenDescriptorPool descriptorPool;
        std::vector<vkb::FrameSpecific<vk::UniqueDescriptorSet>> descriptorSets;
        std::vector<s_ptr<FrameSpecificDescriptorProvider>> descriptorProviders;

        std::vector<RayTracingCall> rayCalls;

        // Final image compositing
        rt::FinalCompositingPass compositingPass;
    };
} // namespace trc
