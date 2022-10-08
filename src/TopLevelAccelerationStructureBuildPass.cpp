#include "trc/TopLevelAccelerationStructureBuildPass.h"

#include "trc/base/Barriers.h"

#include "trc/core/Instance.h"



namespace trc
{

TopLevelAccelerationStructureBuildPass::TopLevelAccelerationStructureBuildPass(
    const Instance& instance,
    rt::TLAS& tlas)
    :
    instance(instance),
    tlas(&tlas),
    scratchBuffer(
        instance.getDevice(),
        glm::max(tlas.getBuildSize().buildScratchSize, tlas.getBuildSize().updateScratchSize),
        nullptr,
        vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress,
        DefaultDeviceMemoryAllocator{ vk::MemoryAllocateFlagBits::eDeviceAddress }
    ),
    scratchMemoryAddress(instance.getDevice()->getBufferAddress({ *scratchBuffer })),
    instanceBuildBuffer(
        instance.getDevice(),
        tlas.getMaxInstances() * sizeof(rt::GeometryInstance),
        vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR
            | vk::BufferUsageFlagBits::eShaderDeviceAddress,
        vk::MemoryPropertyFlagBits::eDeviceLocal | vk::MemoryPropertyFlagBits::eHostVisible,
        DefaultDeviceMemoryAllocator{ vk::MemoryAllocateFlagBits::eDeviceAddress }
    ),
    instances(instanceBuildBuffer.map<rt::GeometryInstance*>())
{
}

void TopLevelAccelerationStructureBuildPass::update(
    vk::CommandBuffer cmdBuf,
    FrameRenderState&)
{
    if (scene != nullptr)
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

        const size_t numInstances = scene->writeTlasInstances(instances);
        instanceBuildBuffer.flush();

        tlas->build(cmdBuf, scratchMemoryAddress, *instanceBuildBuffer, numInstances);
    }
}

void TopLevelAccelerationStructureBuildPass::setScene(DrawableComponentScene& newScene)
{
    scene = &newScene;
}

} // namespace trc
