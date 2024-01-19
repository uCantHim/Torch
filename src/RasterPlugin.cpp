#include "trc/RasterPlugin.h"

#include "trc/GBufferDepthReader.h"
#include "trc/GBufferPass.h"
#include "trc/RasterSceneModule.h"
#include "trc/RasterTasks.h"
#include "trc/TorchRenderStages.h"
#include "trc/core/Frame.h"
#include "trc/core/RenderConfiguration.h"
#include "trc/core/RenderGraph.h"
#include "trc/core/RenderTarget.h"
#include "trc/core/SceneBase.h"
#include "trc/core/Task.h"
#include "trc/TorchImplementation.h"



namespace trc
{

RasterPlugin::RasterPlugin(
    const Device& device,
    s_ptr<AssetDescriptor> assetDescriptor)
    :
    assetDescriptor(assetDescriptor)
    // gBufferDescriptor(std::make_shared<GBufferDescriptor>()),
    // globalDataDescriptor(std::make_shared<GlobalRenderDataDescriptor>()),
    // sceneDescriptor(std::make_shared<SceneDescriptor>()),
    // shadowPool(std::make_shared<ShadowPool>()),

    // gBufferPass(std::make_shared<GBufferPass>()),
    // depthReaderPass(std::make_shared<GBufferDepthReader>()),
    // finalLightingPass(std::make_shared<FinalLightingPass>())
{
    compatibleGBufferRenderPass = GBufferPass::makeVkRenderPass(device);
    compatibleShadowRenderPass = RenderPassShadow::makeVkRenderPass(device);
}

void general_resources(
    const Device& device
    , const FrameClock& clock
    , s_ptr<AssetDescriptor> given
    )
{
    vk::UniqueRenderPass compatGPass = GBufferPass::makeVkRenderPass(device);
    vk::UniqueRenderPass compatShadowPass = RenderPassShadow::makeVkRenderPass(device);

    s_ptr<AssetDescriptor> assetDescriptor = given;
    // s_ptr<GlobalRenderDataDescriptor> globalDataDescriptor;
    GlobalRenderDataDescriptor dataDesc{ device, clock };
    // s_ptr<SceneDescriptor> sceneDescriptor;
    SceneDescriptor sceneDesc{ device };
    // s_ptr<ShadowPool> shadowPool;
    ShadowPool shadowPool{ device, clock, { .maxShadowMaps=200 } };

    s_ptr<GBufferPass> gBufferPass;
    s_ptr<GBufferDepthReader> depthReaderPass;
    s_ptr<FinalLightingPass> finalLightingPass;
}

void per_render_target_resources(
    const Device& device
    , const RenderTarget& target
    )
{
    GBufferCreateInfo createInfo{
        .size=target.getSize(),
        .maxTransparentFragsPerPixel=3
    };
    FrameSpecific<GBuffer> gBuffer{ target.getFrameClock(), device, createInfo };

    GBufferDescriptor gBufferDescriptor{ device, gBuffer };
}

void RasterPlugin::registerRenderStages(RenderGraph& graph)
{
    graph.first(resourceUpdateStage);
    graph.after(resourceUpdateStage, shadowRenderStage);
    graph.after(shadowRenderStage, gBufferRenderStage);
    graph.after(gBufferRenderStage, mouseDepthReadStage);
    graph.after(mouseDepthReadStage, finalLightingRenderStage);
}

void RasterPlugin::defineResources(ResourceConfig& config)
{
    config.defineDescriptor(DescriptorName{ ASSET_DESCRIPTOR },
                            assetDescriptor->getDescriptorSetLayout());
    config.defineDescriptor(DescriptorName{ GLOBAL_DATA_DESCRIPTOR },
                            globalDataDescriptor->getDescriptorSetLayout());
    config.defineDescriptor(DescriptorName{ G_BUFFER_DESCRIPTOR },
                            gBufferDescriptor->getDescriptorSetLayout());
    config.defineDescriptor(DescriptorName{ SCENE_DESCRIPTOR },
                            sceneDescriptor->getDescriptorSetLayout());
    config.defineDescriptor(DescriptorName{ SHADOW_DESCRIPTOR },
                            shadowPool->getDescriptorSetLayout());

    config.addRenderPass(
        RenderPassName{ OPAQUE_G_BUFFER_PASS },
        [&]{ return RenderPassInfo{ *compatibleGBufferRenderPass, 0 }; }
    );
    config.addRenderPass(
        RenderPassName{ TRANSPARENT_G_BUFFER_PASS },
        [&]{ return RenderPassInfo{ *compatibleGBufferRenderPass, 1 }; }
    );
    config.addRenderPass(
        RenderPassName{ SHADOW_PASS },
        [&]{ return RenderPassInfo{ *compatibleShadowRenderPass, 0 }; }
    );
}

void RasterPlugin::registerSceneModules(SceneBase& scene)
{
    scene.registerModule(std::make_unique<RasterSceneModule>());
}

void RasterPlugin::createTasks(SceneBase& scene, TaskQueue& taskQueue)
{
    auto& rasterScene = scene.getModule<RasterSceneModule>();

    // Create shadow tasks
    for (auto& renderPass : rasterScene.getShadowPasses()) {
        taskQueue.spawnTask(shadowRenderStage, std::make_unique<RenderPassDrawTask>(renderPass));
    }

    // G-buffer draw task
    taskQueue.spawnTask(gBufferRenderStage, std::make_unique<RenderPassDrawTask>(gBufferPass));

    // Depth reader task
    taskQueue.spawnTask(mouseDepthReadStage, std::make_unique<RenderPassDrawTask>(depthReaderPass));

    // Final lighting compute task
    taskQueue.spawnTask(finalLightingRenderStage,
                        std::make_unique<RenderPassDrawTask>(finalLightingPass));
}

} // namespace trc
