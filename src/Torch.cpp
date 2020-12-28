#include "Torch.h"

#include "PipelineRegistry.h"
#include "Particle.h" // For particle pipeline creation
#include "text/Text.h"



auto trc::init(const TorchInitInfo& info) -> std::unique_ptr<Renderer>
{
    vkb::vulkanInit({ .deviceExtensions=info.deviceExtensions });

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
