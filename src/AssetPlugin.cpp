#include "trc/AssetPlugin.h"

#include "trc/AssetDescriptor.h"
#include "trc/TorchRenderStages.h"
#include "trc/assets/AssetRegistry.h"
#include "trc/core/Frame.h"
#include "trc/core/RenderGraph.h"
#include "trc/core/ResourceConfig.h"
#include "trc/core/DeviceTask.h"



namespace trc
{

auto buildAssetPlugin(AssetRegistry& reg,
                      const AssetDescriptorCreateInfo& createInfo)
    -> PluginBuilder
{
    return [&reg, createInfo](PluginBuildContext& ctx) {
        return std::make_unique<AssetPlugin>(ctx.instance(), reg, createInfo);
    };
}



AssetPlugin::AssetPlugin(
    const Instance& instance,
    AssetRegistry& registry,
    const AssetDescriptorCreateInfo& createInfo)
    :
    assetDescriptor(makeAssetDescriptor(instance, registry, createInfo)),
    registry(&registry)
{
}

void AssetPlugin::defineRenderStages(RenderGraph& renderGraph)
{
    renderGraph.createOrdering(stages::pre, stages::resourceUpdate);
    renderGraph.createOrdering(stages::resourceUpdate, stages::post);
}

void AssetPlugin::defineResources(ResourceConfig& config)
{
    config.defineDescriptor(DescriptorName{ ASSET_DESCRIPTOR },
                            assetDescriptor->getDescriptorSetLayout());
}

auto AssetPlugin::createGlobalResources(RenderPipelineContext&)
    -> u_ptr<GlobalResources>
{
    return std::make_unique<UpdateConfig>(*this);
}



AssetPlugin::UpdateConfig::UpdateConfig(AssetPlugin& parent)
    :
    parent(&parent)
{
}

void AssetPlugin::UpdateConfig::registerResources(ResourceStorage& resources)
{
    resources.provideDescriptor(DescriptorName{ ASSET_DESCRIPTOR },
                                parent->assetDescriptor);
}

void AssetPlugin::UpdateConfig::hostUpdate(RenderPipelineContext& ctx)
{
    parent->assetDescriptor->update(ctx.device());
}

void AssetPlugin::UpdateConfig::createTasks(GlobalUpdateTaskQueue& taskQueue)
{
    taskQueue.spawnTask(
        stages::resourceUpdate,
        [reg=parent->registry](vk::CommandBuffer cmdBuf, GlobalUpdateContext& ctx) {
            reg->updateDeviceResources(cmdBuf, ctx.frame());
        }
    );
}

} // namespace trc
