#include "ray_tracing/RayTracingPass.h"



void trc::RayTracingPass::begin(vk::CommandBuffer cmdBuf, vk::SubpassContents)
{
    for (auto& f : rayFunctions) {
        f(cmdBuf);
    }
}

void trc::RayTracingPass::addRayFunction(std::function<void(vk::CommandBuffer)> func)
{
    rayFunctions.emplace_back(std::move(func));
}
