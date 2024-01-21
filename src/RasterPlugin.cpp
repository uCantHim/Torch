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



namespace trc
{

RasterPlugin::RasterPlugin(
    const Device& device,
    ui32 maxViewports,
    const RasterPluginCreateInfo& createInfo)
    :
    assetDescriptor(createInfo.assetDescriptor),
    gBufferDescriptor(device, maxViewports),
    globalDataDescriptor(device, maxViewports),
    sceneDescriptor(device),
    shadowDescriptor(createInfo.shadowDescriptor),
    finalLighting(device, maxViewports)
{
    compatibleGBufferRenderPass = GBufferPass::makeVkRenderPass(device);
    compatibleShadowRenderPass = RenderPassShadow::makeVkRenderPass(device);
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
                            globalDataDescriptor.getDescriptorSetLayout());
    config.defineDescriptor(DescriptorName{ G_BUFFER_DESCRIPTOR },
                            gBufferDescriptor.getDescriptorSetLayout());
    config.defineDescriptor(DescriptorName{ SCENE_DESCRIPTOR },
                            sceneDescriptor.getDescriptorSetLayout());
    config.defineDescriptor(DescriptorName{ SHADOW_DESCRIPTOR },
                            shadowDescriptor->getDescriptorSetLayout());

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

auto RasterPlugin::createDrawConfig(const Device& device, Viewport renderTarget)
    -> u_ptr<DrawConfig>
{
    return std::make_unique<RasterDrawConfig>(device, renderTarget, *this);
}

void RasterPlugin::update(SceneBase& scene, const Camera& /*camera*/)
{
    sceneDescriptor.update(scene);
}

void RasterPlugin::createTasks(SceneBase& /*scene*/, TaskQueue& /*taskQueue*/)
{
}



////////////////////////////////////////
//    Per-image draw configuration    //
////////////////////////////////////////

RasterPlugin::RasterDrawConfig::RasterDrawConfig(
    const Device& device,
    Viewport renderTarget,
    RasterPlugin& parent)
    :
    gBuffer(device, { .size=renderTarget.size, .maxTransparentFragsPerPixel=2 }),
    gBufferPass(std::make_shared<GBufferPass>(device, gBuffer)),
    depthReaderPass(std::make_shared<GBufferDepthReader>(
        device,
        []{ return vec2{}; },
        gBuffer
    )),
    finalLighting(parent.finalLighting.makeDrawConfig(device, renderTarget)),
    gBufferDescSet(parent.gBufferDescriptor.makeDescriptorSet(device, gBuffer)),
    globalDataDescSet(parent.globalDataDescriptor.makeDescriptorSet())
{
}

void RasterPlugin::RasterDrawConfig::registerResources(ResourceStorage& resources)
{
    resources.provideDescriptor(DescriptorName{G_BUFFER_DESCRIPTOR},
                                std::make_shared<DescriptorProvider>(*gBufferDescSet));
}

void RasterPlugin::RasterDrawConfig::update(SceneBase& /*scene*/, const Camera& camera)
{
    globalDataDescSet.update(camera);
}

void RasterPlugin::RasterDrawConfig::createTasks(SceneBase& scene, TaskQueue& taskQueue)
{
    auto& rasterScene = scene.getModule<RasterSceneModule>();

    // Shadow tasks - one for each shadow map
    for (auto& renderPass : rasterScene.getShadowPasses()) {
        taskQueue.spawnTask(shadowRenderStage, std::make_unique<RenderPassDrawTask>(renderPass));
    }

    // G-buffer draw task
    taskQueue.spawnTask(gBufferRenderStage,
                        std::make_unique<RenderPassDrawTask>(gBufferPass));

    // Depth reader task
    taskQueue.spawnTask(mouseDepthReadStage,
                        std::make_unique<RenderPassDrawTask>(depthReaderPass));

    // Final lighting compute task
    finalLighting->createTasks(taskQueue);
}

} // namespace trc
