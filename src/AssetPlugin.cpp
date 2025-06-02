#include "trc/AssetPlugin.h"

#include "trc/AssetDescriptor.h"
#include "trc/TorchRenderStages.h"
#include "trc/assets/AssetManager.h"
#include "trc/core/Frame.h"
#include "trc/core/RenderGraph.h"
#include "trc/core/ResourceConfig.h"
#include "trc/core/DeviceTask.h"



namespace trc
{

auto buildAssetPlugin(AssetManager& man,
                      const AssetDescriptorCreateInfo& createInfo)
    -> PluginBuilder
{
    return [&man, createInfo](PluginBuildContext& ctx) {
        return std::make_unique<AssetPlugin>(ctx.instance(), man, createInfo);
    };
}



AssetPlugin::AssetPlugin(
    const Instance& instance,
    AssetManager& manager,
    const AssetDescriptorCreateInfo& createInfo)
    :
    assetDescriptor(makeAssetDescriptor(instance, manager, createInfo)),
    manager(&manager)
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
        [reg=&parent->manager->getDeviceRegistry()](vk::CommandBuffer cmdBuf, GlobalUpdateContext& ctx) {
            reg->updateDeviceResources(cmdBuf, ctx.frame());
        }
    );
}

} // namespace trc
