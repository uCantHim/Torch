#pragma once

#include "trc/core/RenderPipelineTasks.h"
#include "trc/ray_tracing/ShaderBindingTable.h"

namespace trc
{
    class Pipeline;
    class PipelineLayout;

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
    class RayTracingTask : public ViewportDrawTask
    {
    public:
        explicit RayTracingTask(RayTracingCall rayCall);

        void record(vk::CommandBuffer cmdBuf, ViewportDrawContext& ctx) override;

    private:
        RayTracingCall rayCall;
    };
} // namespace trc
