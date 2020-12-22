#include "Torch.h"

#include "PipelineRegistry.h"
#include "Particle.h" // For particle pipeline creation
#include "Text.h"



auto trc::init(const TorchInitInfo& info) -> std::unique_ptr<Renderer>
{
    vkb::vulkanInit();

    auto renderer = std::make_unique<Renderer>(info.rendererInfo);

    // Register required pipelines
    auto& ref = *renderer;
    PipelineRegistry::registerPipeline([&]() { internal::makeAllDrawablePipelines(ref); });
    PipelineRegistry::registerPipeline([&]() { internal::makeFinalLightingPipeline(ref); });
    PipelineRegistry::registerPipeline([&]() {
        internal::makeParticleDrawPipeline(ref);
        internal::makeParticleShadowPipeline(ref);
    });
    PipelineRegistry::registerPipeline([&]() { makeTextPipeline(ref); });

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
