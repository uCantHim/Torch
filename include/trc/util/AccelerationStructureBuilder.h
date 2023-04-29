#pragma once

#include <mutex>
#include <vector>

#include "trc/ray_tracing/AccelerationStructure.h"

namespace trc
{
    class Instance;
    class FrameRenderState;

    class AccelerationStructureBuilder
    {
    public:
        explicit AccelerationStructureBuilder(const Instance& instance);

        void dispatchBuilds(vk::CommandBuffer cmdBuf, FrameRenderState& frame);

        void enqueue(rt::BottomLevelAccelerationStructure& blas);

    private:
        struct BuildInfo
        {
            rt::BottomLevelAccelerationStructure* blas;
            size_t scratchOffset;
        };

        const Instance& instance;
        const size_t scratchMemoryAlignment;

        std::mutex pendingAsLock;
        std::vector<BuildInfo> pendingAccelerationStructures;
        size_t totalScratchSize{ 0 };
    };
} // namespace trc
