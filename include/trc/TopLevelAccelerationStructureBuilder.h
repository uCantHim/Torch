#pragma once

#include "trc/core/Task.h"
#include "trc/ray_tracing/AccelerationStructure.h"
#include "trc/ray_tracing/GeometryUtils.h"

namespace trc
{
    class TopLevelAccelerationStructureBuilder : public Task
    {
    public:
        TopLevelAccelerationStructureBuilder(const Device& device, rt::TLAS& tlas);

        void createTasks(TaskQueue& queue);

    private:
        rt::TLAS* tlas;

        DeviceLocalBuffer scratchBuffer;
        const vk::DeviceAddress scratchMemoryAddress;
        Buffer instanceBuildBuffer;

        /** Persistent mapping of instanceBuildBuffer */
        rt::GeometryInstance* instances;
    };
} // namespace trc
