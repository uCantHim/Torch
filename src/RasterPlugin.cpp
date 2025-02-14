#include "trc/RasterPlugin.h"

#include "trc/GBufferDepthReader.h"
#include "trc/GBufferPass.h"
#include "trc/LightSceneModule.h"
#include "trc/RasterTasks.h"
#include "trc/SceneDescriptor.h"
#include "trc/ShadowPool.h"
#include "trc/TorchRenderStages.h"
#include "trc/core/DeviceTask.h"
#include "trc/core/Frame.h"
#include "trc/core/RenderGraph.h"
#include "trc/core/ResourceConfig.h"
#include "trc/core/SceneBase.h"



namespace trc
{

auto buildRasterPlugin(const RasterPluginCreateInfo& createInfo) -> PluginBuilder
{
    return [createInfo](PluginBuildContext& ctx) {
        return std::make_unique<RasterPlugin>(
            ctx.device(),
            ctx.maxPluginViewportInstances(),
            createInfo
        );
    };
}



RasterPlugin::RasterPlugin(
    const Device& device,
    ui32 maxViewports,
    const RasterPluginCreateInfo& createInfo)
    :
    gBufferDescriptor(device, maxViewports),
    globalDataDescriptor(device, maxViewports),
    sceneDescriptor(std::make_shared<SceneDescriptor>(device)),
    shadowDescriptor(device, createInfo.maxShadowMaps, maxViewports),
    finalLighting(device, maxViewports)
{
    compatibleGBufferRenderPass = GBufferPass::makeVkRenderPass(device);
    compatibleShadowRenderPass = RenderPassShadow::makeVkRenderPass(device);
}

void RasterPlugin::defineRenderStages(RenderGraph& graph)
{
    graph.insert(stages::shadow);
    graph.insert(stages::gBuffer);
    graph.insert(stages::deferredLighting);

    // Internal deps
    graph.createOrdering(stages::resourceUpdate, stages::shadow);
    graph.createOrdering(stages::resourceUpdate, stages::gBuffer);
    graph.createOrdering(stages::shadow, stages::deferredLighting);
    graph.createOrdering(stages::gBuffer, stages::deferredLighting);

    // Deps to pre and post
    graph.createOrdering(stages::pre, stages::deferredLighting);
    graph.createOrdering(stages::deferredLighting, stages::post);
}

void RasterPlugin::defineResources(ResourceConfig& config)
{
    config.defineDescriptor(DescriptorName{ GLOBAL_DATA_DESCRIPTOR },
                            globalDataDescriptor.getDescriptorSetLayout());
    config.defineDescriptor(DescriptorName{ G_BUFFER_DESCRIPTOR },
                            gBufferDescriptor.getDescriptorSetLayout());
    config.defineDescriptor(DescriptorName{ SCENE_DESCRIPTOR },
                            sceneDescriptor->getDescriptorSetLayout());
    config.defineDescriptor(DescriptorName{ SHADOW_DESCRIPTOR },
                            shadowDescriptor.getDescriptorSetLayout());
    config.defineDescriptor(DescriptorName{ FinalLighting::OUTPUT_IMAGE_DESCRIPTOR },
                            finalLighting.getDescriptorSetLayout());

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

auto RasterPlugin::createSceneResources(SceneContext& ctx)
    -> u_ptr<SceneResources>
{
    return std::make_unique<SceneConfig>(ctx, *this);
}

auto RasterPlugin::createViewportResources(ViewportContext& ctx)
    -> u_ptr<ViewportResources>
{
    return std::make_unique<DrawConfig>(ctx, *this);
}



////////////////////////////////////////
//    Per-image draw configuration    //
////////////////////////////////////////

RasterPlugin::DrawConfig::DrawConfig(
    ViewportContext& ctx,
    RasterPlugin& parent)
    :
    parent(&parent),
    gBuffer(ctx.device(), {
        .size=ctx.renderArea().size,
        .maxTransparentFragsPerPixel=parent.config.maxTransparentFragsPerPixel,
    }),
    gBufferPass(std::make_shared<GBufferPass>(ctx.device(), gBuffer)),
    gBufferDepthReaderPass(
        parent.config.depthReaderCallback
        ? std::make_shared<GBufferDepthReader>(
            ctx.device(),
            parent.config.depthReaderCallback,
            *gBuffer.getImage(GBuffer::Image::eDepth))
        : nullptr
    ),
    finalLighting(parent.finalLighting.makeDrawConfig(ctx.device(), ctx.viewport())),
    gBufferDescSet(parent.gBufferDescriptor.makeDescriptorSet(ctx.device(), gBuffer)),
    globalDataDescriptor(std::make_shared<GlobalRenderDataDescriptor::DescriptorSet>(
        parent.globalDataDescriptor.makeDescriptorSet()
    ))
{
    gBufferPass->setClearColor(ctx.clearColor());
}

void RasterPlugin::DrawConfig::registerResources(ResourceStorage& resources)
{
    resources.provideDescriptor(DescriptorName{ GLOBAL_DATA_DESCRIPTOR },
                                globalDataDescriptor);
    resources.provideDescriptor(DescriptorName{ G_BUFFER_DESCRIPTOR },
                                std::make_shared<DescriptorProvider>(*gBufferDescSet));
    resources.provideDescriptor(DescriptorName{ SCENE_DESCRIPTOR },
                                parent->sceneDescriptor);
}

void RasterPlugin::DrawConfig::hostUpdate(ViewportContext& ctx)
{
    parent->sceneDescriptor->update(ctx.scene());
    globalDataDescriptor->update(ctx.camera());
}

void RasterPlugin::DrawConfig::createTasks(
    ViewportDrawTaskQueue& queue,
    ViewportContext&)
{
    // G-buffer draw task
    queue.spawnTask(
        stages::gBuffer,
        std::make_unique<RenderPassDrawTask>(stages::gBuffer, gBufferPass)
    );

    // Depth reader task
    if (gBufferDepthReaderPass)
    {
        queue.spawnTask(stages::gBuffer,
            [this](vk::CommandBuffer cmdBuf, ViewportDrawContext& ctx) {
                gBufferDepthReaderPass->update(cmdBuf, ctx);
            }
        );
    }

    // Final lighting compute task
    finalLighting->createTasks(queue);
}



RasterPlugin::SceneConfig::SceneConfig(SceneContext& ctx, RasterPlugin& parent)
    :
    shadowPool(
        ctx.device(),
        ShadowPoolCreateInfo{ .maxShadowMaps=parent.config.maxShadowMaps }
    ),
    shadowDescriptorSet(
        parent.shadowDescriptor.makeDescriptorSet(ctx.device(), shadowPool)
    )
{
    auto& lights = ctx.scene().getModule<LightSceneModule>();
    onShadowCreate = lights.onShadowCreate(
        [this](const ShadowRegistry::ShadowCreateEvent& event)
        {
            auto shadowMap = shadowPool.allocateShadow(
                event.id,
                { event.createInfo.shadowMapResolution, event.createInfo.camera, }
            );
            allocatedShadows.emplace(event.id, std::move(shadowMap));
        }
    );
    onShadowDestroy = lights.onShadowDestroy(
        [this](const ShadowRegistry::ShadowDestroyEvent& event) {
            allocatedShadows.erase(event.id);
        }
    );
}

void RasterPlugin::SceneConfig::registerResources(ResourceStorage& resources)
{
    resources.provideDescriptor(DescriptorName{ SHADOW_DESCRIPTOR },
                                shadowDescriptorSet);
}

void RasterPlugin::SceneConfig::hostUpdate(SceneContext& ctx)
{
    shadowPool.update();
    shadowDescriptorSet->update(ctx.device(), shadowPool);
}

void RasterPlugin::SceneConfig::createTasks(SceneUpdateTaskQueue& taskQueue)
{
    for (auto& [_, shadowMap] : allocatedShadows)
    {
        taskQueue.spawnTask(
            stages::shadow,
            std::make_unique<ShadowMapDrawTask>(stages::shadow, shadowMap)
        );
    }
}

} // namespace trc
