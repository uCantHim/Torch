#include "trc/ray_tracing/RayTracingPass.h"

#include "trc/core/Pipeline.h"
#include "trc/ray_tracing/ShaderBindingTable.h"



trc::RayTracingTask::RayTracingTask(RayTracingCall rayCall)
    :
    rayCall(rayCall)
{
}

void trc::RayTracingTask::record(vk::CommandBuffer cmdBuf, ViewportDrawContext& ctx)
{
    ctx.deps().consume(ImageAccess{
        rayCall.outputImage,
        vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1),
        vk::PipelineStageFlagBits2::eRayTracingShaderKHR,
        vk::AccessFlagBits2::eShaderStorageRead | vk::AccessFlagBits2::eShaderStorageWrite,
        vk::ImageLayout::eGeneral,
    });

    rayCall.pipeline->bind(cmdBuf, ctx.resources());

    cmdBuf.traceRaysKHR(
        rayCall.sbt->getEntryAddress(rayCall.raygenTableIndex),
        rayCall.sbt->getEntryAddress(rayCall.missTableIndex),
        rayCall.sbt->getEntryAddress(rayCall.hitTableIndex),
        rayCall.sbt->getEntryAddress(rayCall.callableTableIndex),
        rayCall.viewportSize.x, rayCall.viewportSize.y, 1,
        ctx.device().getDL()
    );

    //ctx.deps().produce(ImageAccess{
    //    rayCall.outputImage,
    //    vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1),
    //    vk::PipelineStageFlagBits2::eRayTracingShaderKHR,
    //    vk::AccessFlagBits2::eShaderStorageWrite,
    //    vk::ImageLayout::eGeneral,
    //});
}
