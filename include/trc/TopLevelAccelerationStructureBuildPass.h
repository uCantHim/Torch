#pragma once

#include "UpdatePass.h"
#include "drawable/DrawableComponentScene.h"
#include "ray_tracing/AccelerationStructure.h"

namespace trc
{
    class TopLevelAccelerationStructureBuildPass : public UpdatePass
    {
    public:
        TopLevelAccelerationStructureBuildPass(const Instance& instance, rt::TLAS& tlas);

        void update(vk::CommandBuffer cmdBuf, FrameRenderState& frame) override;

        void setScene(DrawableComponentScene& scene);

    private:
        const Instance& instance;
        rt::TLAS* tlas;

        DrawableComponentScene* scene{ nullptr };

        vkb::DeviceLocalBuffer scratchBuffer;
        const vk::DeviceAddress scratchMemoryAddress;
        vkb::Buffer instanceBuildBuffer;
        /** Persistent mapping of instanceBuildBuffer */
        rt::GeometryInstance* instances;
    };
} // namespace trc
