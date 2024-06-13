#include "trc/ray_tracing/RayTracingPlugin.h"

#include "trc/AssetPlugin.h"
#include "trc/RasterPlugin.h"
#include "trc/RayShaders.h"
#include "trc/TorchRenderStages.h"
#include "trc/core/PipelineLayoutBuilder.h"
#include "trc/core/RenderGraph.h"
#include "trc/core/ResourceConfig.h"



namespace trc
{

RayTracingPlugin::RayTracingPlugin(
    const Instance& instance,
    ResourceConfig& resourceConfig,
    ui32 maxViewports,
    ui32 tlasMaxInstances)
    :
    maxTlasInstances(tlasMaxInstances),
    instance(instance),
    raygenDescriptorPool(instance, rt::RayBuffer::Image::NUM_IMAGES * maxViewports),
    compositingDescriptorPool(instance.getDevice(), maxViewports),
    reflectPipelineLayout(trc::buildPipelineLayout()
        .addDynamicDescriptor(raygenDescriptorPool.getDescriptorSetLayout())
        .addDescriptor(DescriptorName{ RasterPlugin::G_BUFFER_DESCRIPTOR }, true)
        .addDescriptor(DescriptorName{ AssetPlugin::ASSET_DESCRIPTOR }, true)
        .addDescriptor(DescriptorName{ RasterPlugin::SCENE_DESCRIPTOR }, true)
        .addDescriptor(DescriptorName{ RasterPlugin::SHADOW_DESCRIPTOR }, true)
        .addDescriptor(DescriptorName{ RasterPlugin::GLOBAL_DATA_DESCRIPTOR }, true)
        .build(instance.getDevice(), resourceConfig)
    ),
    sbtMemoryPool(instance.getDevice(), 20000000)  // 20 MiB
{
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
        .build(kMaxRecursionDepth, reflectPipelineLayout, sbtMemoryPool.makeAllocator());

    this->reflectPipeline = std::make_shared<Pipeline>(std::move(reflectPipeline));
    this->reflectShaderBindingTable = std::make_unique<rt::ShaderBindingTable>(std::move(sbt));
}

void RayTracingPlugin::registerRenderStages(RenderGraph& graph)
{
    graph.insert(rayTracingRenderStage);
    graph.insert(finalCompositingRenderStage);

    graph.createOrdering(resourceUpdateStage, rayTracingRenderStage);
    graph.createOrdering(rayTracingRenderStage, finalCompositingRenderStage);
    graph.createOrdering(finalLightingRenderStage, finalCompositingRenderStage);
}

void RayTracingPlugin::defineResources(ResourceConfig& /*conf*/)
{
}

auto RayTracingPlugin::createDrawConfig(const Device& /*device*/, Viewport renderTarget)
    -> u_ptr<DrawConfig>
{
    return std::make_unique<RayDrawConfig>(*this, renderTarget);
}



RayTracingPlugin::RayDrawConfig::RayDrawConfig(
    RayTracingPlugin& parent,
    Viewport renderTarget)
    :
    rayBuffer{
        parent.instance.getDevice(),
        rt::RayBufferCreateInfo{
            .size=renderTarget.size,
            .imageUsage{},
        }
    },
    tlas{ parent.instance, parent.maxTlasInstances },
    raygenDescriptor{
        parent.raygenDescriptorPool.allocateDescriptorSet(
            tlas,
            rayBuffer.getImageView(rt::RayBuffer::Image::eReflections)
        )
    },
    tlasBuilder(parent.instance.getDevice(), tlas),
    compositingPass{
        parent.instance.getDevice(),
        rayBuffer,
        renderTarget,
        parent.compositingDescriptorPool
    },
    reflectionsRayCall{
        .pipelineLayout = &parent.reflectPipelineLayout,
        .pipeline       = parent.reflectPipeline.get(),
        .sbt            = parent.reflectShaderBindingTable.get(),
        .raygenTableIndex   = 0,
        .missTableIndex     = 1,
        .hitTableIndex      = 2,
        .callableTableIndex = {},
        .raygenDescriptorSet{ *raygenDescriptor },
        .outputImage = *rayBuffer.getImage(trc::rt::RayBuffer::Image::eReflections),
        .viewportSize{ renderTarget.size },
    }
{
}

void RayTracingPlugin::RayDrawConfig::registerResources(ResourceStorage& /*resources*/)
{
}

void RayTracingPlugin::RayDrawConfig::update(
    const Device& /*device*/,
    SceneBase& /*scene*/,
    const Camera& /*camera*/)
{
}

void RayTracingPlugin::RayDrawConfig::createTasks(SceneBase& /*scene*/, TaskQueue& taskQueue)
{
    tlasBuilder.createTasks(taskQueue);
    taskQueue.spawnTask(
        rayTracingRenderStage,
        std::make_unique<RayTracingTask>(reflectionsRayCall)
    );
    compositingPass.createTasks(taskQueue);
}

} // namespace trc
