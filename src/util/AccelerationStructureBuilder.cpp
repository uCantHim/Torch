#include "trc/util/AccelerationStructureBuilder.h"

#include <trc_util/Padding.h>
#include <trc_util/algorithm/VectorTransform.h>
#include "trc/base/Barriers.h"

#include "trc/core/Frame.h"
#include "trc/core/Instance.h"



trc::AccelerationStructureBuilder::AccelerationStructureBuilder(const Instance& instance)
    :
    instance(instance),
    scratchMemoryAlignment(
        instance.getPhysicalDevice()->getProperties2<
            vk::PhysicalDeviceProperties2,
            vk::PhysicalDeviceAccelerationStructurePropertiesKHR
        >().get<vk::PhysicalDeviceAccelerationStructurePropertiesKHR>()
            .minAccelerationStructureScratchOffsetAlignment
    )
{
}

void trc::AccelerationStructureBuilder::dispatchBuilds(vk::CommandBuffer cmdBuf, FrameRenderState& frame)
{
    std::scoped_lock lock(pendingAsLock);

    if (pendingAccelerationStructures.empty()) {
        return;
    }

    auto& scratchBuffer = frame.makeTransientBuffer(
        instance.getDevice(),
        totalScratchSize,
        vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eStorageBuffer,
        vk::MemoryPropertyFlagBits::eDeviceLocal,
        DefaultDeviceMemoryAllocator{ vk::MemoryAllocateFlagBits::eDeviceAddress }
    );
    const auto scratchMemoryAddress = instance.getDevice()->getBufferAddress({ *scratchBuffer });

    std::vector<const vk::AccelerationStructureBuildRangeInfoKHR*> buildRangePointers;
    std::vector<u_ptr<std::vector<vk::AccelerationStructureBuildRangeInfoKHR>>> buildRanges;
    std::vector<vk::AccelerationStructureBuildGeometryInfoKHR> geoBuildInfos;
    for (auto [blas, offset] : pendingAccelerationStructures)
    {
        auto [ranges, ptrs] = blas->makeBuildRanges();
        buildRanges.emplace_back(std::move(ranges));
        util::merge(buildRangePointers, ptrs);
        geoBuildInfos.emplace_back(blas->getGeometryBuildInfo())
            .setScratchData(scratchMemoryAddress + offset)
            .setDstAccelerationStructure(**blas);
    }

    cmdBuf.buildAccelerationStructuresKHR(geoBuildInfos, buildRangePointers, instance.getDL());

    pendingAccelerationStructures.clear();
    totalScratchSize = 0;
}

void trc::AccelerationStructureBuilder::enqueue(rt::BottomLevelAccelerationStructure& blas)
{
    std::scoped_lock lock(pendingAsLock);

    pendingAccelerationStructures.push_back({ &blas, totalScratchSize });
    totalScratchSize += util::pad(blas.getBuildSize().buildScratchSize, scratchMemoryAlignment);
}
