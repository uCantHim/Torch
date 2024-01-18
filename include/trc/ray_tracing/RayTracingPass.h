#pragma once

#include <vector>

#include "trc/base/FrameSpecificObject.h"

#include "trc/core/Pipeline.h"
#include "trc/core/RenderPass.h"
#include "trc/ray_tracing/AccelerationStructure.h"
#include "trc/ray_tracing/FinalCompositingPass.h"
#include "trc/ray_tracing/RaygenDescriptor.h"
#include "trc/ray_tracing/RayBuffer.h"
#include "trc/ray_tracing/RayPipelineBuilder.h"

namespace trc
{
    class RenderConfig;
    class RenderTarget;

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
                       RenderConfig& renderConfig,
                       const rt::TLAS& tlas,
                       const FrameSpecific<rt::RayBuffer>* rayBuffer);

        void begin(vk::CommandBuffer, vk::SubpassContents, FrameRenderState&) override;
        void end(vk::CommandBuffer) override;

        void setRayBuffer(const FrameSpecific<rt::RayBuffer>* rayBuffer);

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

        static constexpr ui32 kMaxReccursionDepth{ 16 };

        const Instance& instance;
        const rt::TLAS& tlas;

        const FrameSpecific<rt::RayBuffer>* rayBuffer;

        rt::RaygenDescriptorPool descriptorPool;
        std::vector<FrameSpecific<vk::UniqueDescriptorSet>> descriptorSets;
        std::vector<s_ptr<FrameSpecificDescriptorProvider>> descriptorProviders;

        std::vector<RayTracingCall> rayCalls;
    };
} // namespace trc
