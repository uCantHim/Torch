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

    void* deviceFeatureChain{ nullptr };

    // Ray tracing device features
    vk::StructureChain deviceFeatures{
        vk::PhysicalDeviceFeatures2{}, // required for chain validity
        vk::PhysicalDeviceBufferDeviceAddressFeatures{},
        vk::PhysicalDeviceAccelerationStructureFeaturesKHR{},
        vk::PhysicalDeviceRayTracingPipelineFeaturesKHR{}
    };
    if (info.enableRayTracing)
    {
        // Add ray tracing features to feature chain
        deviceFeatureChain = &deviceFeatures.get<vk::PhysicalDeviceBufferDeviceAddressFeatures>();
    }

    // Initialize vkb
    vkb::vulkanInit({
        .deviceExtensions                = deviceExtensions,
        .extraPhysicalDeviceFeatureChain = deviceFeatureChain
    });
    Queues::init(vkb::getQueueManager());

    auto renderer = std::make_unique<Renderer>(info.rendererInfo);

    // Register required pipelines
    PipelineRegistry::registerPipeline([&]() {
        RenderPassShadow dummyPass{{ 1, 1 }};
        internal::makeParticleShadowPipeline(*dummyPass);
    });
    PipelineRegistry::registerPipeline([]() {
        auto renderPass = RenderPassDeferred::makeVkRenderPassInstance(vkb::getSwapchain());

        internal::makeAllDrawablePipelines(*renderPass);
        internal::makeFinalLightingPipeline(*renderPass);
        internal::makeParticleDrawPipeline(*renderPass);
        makeTextPipeline(*renderPass);
    });

    // Create all pipelines for the first time
    PipelineRegistry::recreateAll();

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
