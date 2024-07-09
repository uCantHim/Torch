#pragma once

#include "trc/core/RenderPipelineTasks.h"
#include "trc/ray_tracing/AccelerationStructure.h"
#include "trc/ray_tracing/GeometryUtils.h"

namespace trc
{
    class RaySceneModule;

    class TopLevelAccelerationStructureBuilder
    {
    public:
        TopLevelAccelerationStructureBuilder(const Device& device, rt::TLAS& tlas);

        void uploadData(RaySceneModule& scene);
        void createBuildTasks(SceneUpdateTaskQueue& queue);

    private:
        rt::TLAS* tlas;

        DeviceLocalBuffer scratchBuffer;
        const vk::DeviceAddress scratchMemoryAddress;
        Buffer instanceBuildBuffer;

        /** Persistent mapping of instanceBuildBuffer */
        rt::GeometryInstance* instances;
        size_t numInstances{ 0 };
    };
} // namespace trc
