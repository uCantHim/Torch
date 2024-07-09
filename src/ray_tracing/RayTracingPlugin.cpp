#include "trc/ray_tracing/RayTracingPlugin.h"

#include "trc/AssetPlugin.h"
#include "trc/RasterPlugin.h"
#include "trc/RaySceneModule.h"
#include "trc/RayShaders.h"
#include "trc/TorchRenderStages.h"
#include "trc/core/PipelineLayoutBuilder.h"
#include "trc/core/RenderGraph.h"
#include "trc/core/ResourceConfig.h"
#include "trc/core/SceneBase.h"
#include "trc/ray_tracing/RayPipelineBuilder.h"



namespace trc
{

constexpr size_t _20MiB{ 20000000 };



auto buildRayTracingPlugin(PluginBuildContext& ctx) -> u_ptr<RenderPlugin>
{

}



RayTracingPlugin::RayTracingPlugin(
    const Instance& instance,
    ui32 maxViewports,
    ui32 tlasMaxInstances)
    :
    maxTlasInstances(tlasMaxInstances),
    instance(instance),
    raygenDescriptorPool(instance, rt::RayBuffer::Image::NUM_IMAGES * maxViewports),
    compositingDescriptorPool(instance.getDevice(), maxViewports),
    reflectPipelineLayout(nullptr),
    sbtMemoryPool(instance.getDevice(), _20MiB)
{
}

void RayTracingPlugin::defineRenderStages(RenderGraph& graph)
{
    graph.insert(rayTracingRenderStage);
    graph.insert(finalCompositingRenderStage);

    graph.createOrdering(resourceUpdateStage, rayTracingRenderStage);
    graph.createOrdering(rayTracingRenderStage, finalCompositingRenderStage);
    graph.createOrdering(finalLightingRenderStage, finalCompositingRenderStage);
}

void RayTracingPlugin::defineResources(ResourceConfig& conf)
{
    conf.defineDescriptor({ RAYGEN_TLAS_DESCRIPTOR },
                          raygenDescriptorPool.getTlasDescriptorSetLayout());
    conf.defineDescriptor({ RAYGEN_IMAGE_DESCRIPTOR },
                          raygenDescriptorPool.getImageDescriptorSetLayout());
}

auto RayTracingPlugin::createViewportResources(ViewportContext& ctx)
    -> u_ptr<ViewportResources>
{
    if (!isInitialized) {
        init(ctx);
        isInitialized = true;
    }

    return std::make_unique<RayDrawConfig>(
        *this,
        Viewport{ ctx.renderImage(), ctx.renderArea() }
    );
}

void RayTracingPlugin::init(ViewportContext& ctx)
{
    reflectPipelineLayout = std::make_unique<PipelineLayout>(
        trc::buildPipelineLayout()
        .addDescriptor(DescriptorName{ RAYGEN_TLAS_DESCRIPTOR }, true)
        .addDescriptor(DescriptorName{ RAYGEN_IMAGE_DESCRIPTOR }, true)
        .addDescriptor(DescriptorName{ RasterPlugin::G_BUFFER_DESCRIPTOR }, true)
        .addDescriptor(DescriptorName{ AssetPlugin::ASSET_DESCRIPTOR }, true)
        .addDescriptor(DescriptorName{ RasterPlugin::SCENE_DESCRIPTOR }, true)
        .addDescriptor(DescriptorName{ RasterPlugin::SHADOW_DESCRIPTOR }, true)
        .addDescriptor(DescriptorName{ RasterPlugin::GLOBAL_DATA_DESCRIPTOR }, true)
        .build(instance.getDevice(), ctx.resourceConfig())
    );

    // ------------------------------- //
    //    Make reflections pipeline    //
    // ------------------------------- //
    auto [reflectPipeline, sbt] =
        trc::rt::buildRayTracingPipeline(instance)
        .addRaygenGroup(rt::shaders::getReflectRaygen())
        .beginTableEntry()
            .addMissGroup(rt::shaders::getBlueMiss())
        .endTableEntry()
        .addTrianglesHitGroup(rt::shaders::getReflectHit(), rt::shaders::getAnyhit())
        .build(kMaxRecursionDepth, *reflectPipelineLayout, sbtMemoryPool.makeAllocator());

    this->reflectPipeline = std::make_shared<Pipeline>(std::move(reflectPipeline));
    this->reflectShaderBindingTable = std::make_unique<rt::ShaderBindingTable>(std::move(sbt));
}



RayTracingPlugin::TlasUpdateConfig::TlasUpdateConfig(RayTracingPlugin& parent)
    :
    tlas{ parent.instance, parent.maxTlasInstances },
    tlasBuilder(parent.instance.getDevice(), tlas)
{
}

void RayTracingPlugin::TlasUpdateConfig::registerResources(ResourceStorage& resources)
{
    resources.provideDescriptor(
        { RAYGEN_TLAS_DESCRIPTOR },
        std::make_shared<DescriptorProvider>(*tlasDescriptor)
    );
}

void RayTracingPlugin::TlasUpdateConfig::hostUpdate(SceneContext& ctx)
{
    tlasBuilder.uploadData(ctx.scene().getModule<RaySceneModule>());
}

void RayTracingPlugin::TlasUpdateConfig::createTasks(SceneUpdateTaskQueue& taskQueue)
{
    tlasBuilder.createBuildTasks(taskQueue);
}



RayTracingPlugin::RayDrawConfig::RayDrawConfig(
    RayTracingPlugin& parent,
    Viewport renderTarget)
    :
    rayBuffer{
        parent.instance.getDevice(),
        rt::RayBufferCreateInfo{
            .size=renderTarget.area.size,
            .imageUsage{},
        }
    },
    outputImageDescriptor{
        parent.raygenDescriptorPool.allocateImageDescriptorSet(renderTarget.target.imageView)
    },
    compositingPass{
        parent.instance.getDevice(),
        rayBuffer,
        renderTarget,
        parent.compositingDescriptorPool
    },
    reflectionsRayCall{
        .pipelineLayout = parent.reflectPipelineLayout.get(),
        .pipeline       = parent.reflectPipeline.get(),
        .sbt            = parent.reflectShaderBindingTable.get(),
        .raygenTableIndex   = 0,
        .missTableIndex     = 1,
        .hitTableIndex      = 2,
        .callableTableIndex = {},
        .raygenDescriptorSet{ *outputImageDescriptor },
        .outputImage = *rayBuffer.getImage(trc::rt::RayBuffer::Image::eReflections),
        .viewportSize{ renderTarget.area.size },
    }
{
}

void RayTracingPlugin::RayDrawConfig::registerResources(ResourceStorage& resources)
{
    resources.provideDescriptor(
        { RAYGEN_TLAS_DESCRIPTOR },
        std::make_shared<DescriptorProvider>(*outputImageDescriptor)
    );
}

void RayTracingPlugin::RayDrawConfig::createTasks(
    ViewportDrawTaskQueue& taskQueue,
    ViewportContext&)
{
    taskQueue.spawnTask(
        rayTracingRenderStage,
        std::make_unique<RayTracingTask>(reflectionsRayCall)
    );
    compositingPass.createTasks(taskQueue);
}

} // namespace trc
