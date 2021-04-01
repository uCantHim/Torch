#include "Torch.h"

#include "PipelineRegistry.h"
#include "Particle.h" // For particle pipeline creation
#include "text/Text.h"



auto trc::init(const TorchInitInfo& info) -> std::unique_ptr<Renderer>
{
    auto deviceExtensions = info.deviceExtensions;
    if (info.enableRayTracing)
    {
        deviceExtensions.push_back(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME);
        deviceExtensions.push_back(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME);
        deviceExtensions.push_back(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME);
        deviceExtensions.push_back(VK_KHR_RAY_QUERY_EXTENSION_NAME);
    }

    vkb::vulkanInit({
        .deviceExtensions         = deviceExtensions,
        .enableRayTracingFeatures = info.enableRayTracing
    });
    Queues::init(vkb::getQueueManager());

    auto renderer = std::make_unique<Renderer>(info.rendererInfo);

    return renderer;
}

void trc::terminate()
{
    vkb::getDevice()->waitIdle();

    AssetRegistry::reset();
    RenderPass::destroyAll();
    Pipeline::destroyAll();
    RenderStageType::destroyAll();
    vkb::vulkanTerminate();
}
