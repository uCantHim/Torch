#pragma once

#include <vector>

#include "trc/base/FrameSpecificObject.h"

#include "trc/core/Pipeline.h"
#include "trc/core/Task.h"
#include "trc/ray_tracing/AccelerationStructure.h"
#include "trc/ray_tracing/FinalCompositingPass.h"
#include "trc/ray_tracing/RaygenDescriptor.h"
#include "trc/ray_tracing/RayBuffer.h"
#include "trc/ray_tracing/RayPipelineBuilder.h"

namespace trc
{
    struct RayTracingCall
    {
        PipelineLayout* pipelineLayout;
        Pipeline* pipeline;

        rt::ShaderBindingTable* sbt;

        ui32 raygenTableIndex;
        ui32 missTableIndex;
        ui32 hitTableIndex;
        ui32 callableTableIndex;

        vk::DescriptorSet raygenDescriptorSet;
        vk::Image outputImage;
        uvec2 viewportSize;
    };

    /**
     * @brief A renderpass that does nothing
     */
    class RayTracingTask : public Task
    {
    public:
        explicit RayTracingTask(RayTracingCall rayCall);

        void record(vk::CommandBuffer cmdBuf, TaskEnvironment& env) override;

    private:
        RayTracingCall rayCall;
    };
} // namespace trc
