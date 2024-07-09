#include "trc/TopLevelAccelerationStructureBuilder.h"

#include "trc/RaySceneModule.h"
#include "trc/TorchRenderStages.h"
#include "trc/base/Barriers.h"
#include "trc/core/SceneBase.h"
#include "trc/core/DeviceTask.h"



namespace trc
{

TopLevelAccelerationStructureBuilder::TopLevelAccelerationStructureBuilder(
    const Device& device,
    rt::TLAS& tlas)
    :
    tlas(&tlas),
    scratchBuffer(
        device,
        glm::max(tlas.getBuildSize().buildScratchSize, tlas.getBuildSize().updateScratchSize),
        nullptr,
        vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress,
        DefaultDeviceMemoryAllocator{ vk::MemoryAllocateFlagBits::eDeviceAddress }
    ),
    scratchMemoryAddress(device->getBufferAddress({ *scratchBuffer })),
    instanceBuildBuffer(
        device,
        tlas.getMaxInstances() * sizeof(rt::GeometryInstance),
        vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR
            | vk::BufferUsageFlagBits::eShaderDeviceAddress,
        vk::MemoryPropertyFlagBits::eDeviceLocal | vk::MemoryPropertyFlagBits::eHostVisible,
        DefaultDeviceMemoryAllocator{ vk::MemoryAllocateFlagBits::eDeviceAddress }
    ),
    instances(instanceBuildBuffer.map<rt::GeometryInstance*>())
{
}

void TopLevelAccelerationStructureBuilder::uploadData(RaySceneModule& scene)
{
    numInstances = scene.writeTlasInstances(instances, tlas->getMaxInstances());
    instanceBuildBuffer.flush();
}

void TopLevelAccelerationStructureBuilder::createBuildTasks(SceneUpdateTaskQueue& queue)
{
    queue.spawnTask(
        resourceUpdateStage,
        [this](vk::CommandBuffer cmdBuf, auto&&)
        {
            bufferMemoryBarrier(
                cmdBuf,
                *scratchBuffer, 0, VK_WHOLE_SIZE,
                vk::PipelineStageFlagBits::eAccelerationStructureBuildKHR,
                vk::PipelineStageFlagBits::eAccelerationStructureBuildKHR,
                vk::AccessFlagBits::eAccelerationStructureReadKHR,
                vk::AccessFlagBits::eAccelerationStructureReadKHR
            );
            bufferMemoryBarrier(
                cmdBuf,
                *instanceBuildBuffer, 0, VK_WHOLE_SIZE,
                vk::PipelineStageFlagBits::eHost,
                vk::PipelineStageFlagBits::eAccelerationStructureBuildKHR,
                vk::AccessFlagBits::eMemoryWrite,
                vk::AccessFlagBits::eShaderRead
            );

            tlas->build(cmdBuf, scratchMemoryAddress, *instanceBuildBuffer, numInstances);
        }
    );
}

} // namespace trc
