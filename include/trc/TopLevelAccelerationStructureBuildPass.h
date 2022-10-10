#pragma once

#include "trc/UpdatePass.h"
#include "trc/drawable/DrawableComponentScene.h"
#include "trc/ray_tracing/AccelerationStructure.h"

namespace trc
{
    class TopLevelAccelerationStructureBuildPass : public UpdatePass
    {
    public:
        TopLevelAccelerationStructureBuildPass(const Instance& instance, rt::TLAS& tlas);

        void update(vk::CommandBuffer cmdBuf, FrameRenderState& frame) override;

        void setScene(const DrawableComponentScene& scene);

    private:
        const Instance& instance;
        rt::TLAS* tlas;

        const DrawableComponentScene* scene{ nullptr };

        DeviceLocalBuffer scratchBuffer;
        const vk::DeviceAddress scratchMemoryAddress;
        Buffer instanceBuildBuffer;
        /** Persistent mapping of instanceBuildBuffer */
        rt::GeometryInstance* instances;
    };
} // namespace trc
