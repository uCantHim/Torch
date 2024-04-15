#include "trc/ray_tracing/RayTracingPass.h"

#include "trc/base/Barriers.h"



trc::RayTracingTask::RayTracingTask(RayTracingCall rayCall)
    :
    rayCall(rayCall)
{
}

void trc::RayTracingTask::record(vk::CommandBuffer cmdBuf, TaskEnvironment& env)
{
    barrier(cmdBuf,
        vk::ImageMemoryBarrier2(
            vk::PipelineStageFlagBits2::eNone,
            vk::AccessFlagBits2::eNone,
            vk::PipelineStageFlagBits2::eRayTracingShaderKHR,
            vk::AccessFlagBits2::eShaderStorageRead | vk::AccessFlagBits2::eShaderStorageWrite,
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::eGeneral,
            VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
            rayCall.outputImage,
            vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1)
        )
    );

    cmdBuf.bindDescriptorSets(vk::PipelineBindPoint::eRayTracingKHR,
                              **rayCall.pipelineLayout,
                              0, { rayCall.raygenDescriptorSet }, {});
    rayCall.pipeline->bind(cmdBuf, *env.resources);

    cmdBuf.traceRaysKHR(
        rayCall.sbt->getEntryAddress(rayCall.raygenTableIndex),
        rayCall.sbt->getEntryAddress(rayCall.missTableIndex),
        rayCall.sbt->getEntryAddress(rayCall.hitTableIndex),
        rayCall.sbt->getEntryAddress(rayCall.callableTableIndex),
        rayCall.viewportSize.x, rayCall.viewportSize.y, 1,
        env.device.getDL()
    );
    barrier(cmdBuf,
        vk::ImageMemoryBarrier2(
            vk::PipelineStageFlagBits2::eRayTracingShaderKHR,
            vk::AccessFlagBits2::eShaderStorageWrite,
            vk::PipelineStageFlagBits2::eComputeShader,
            vk::AccessFlagBits2::eShaderStorageRead,
            vk::ImageLayout::eGeneral,
            vk::ImageLayout::eGeneral,
            VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
            rayCall.outputImage,
            vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1)
        )
    );
}
