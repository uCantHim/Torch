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

AssetPlugin::AssetPlugin(
    AssetRegistry& registry,
    s_ptr<AssetDescriptor> assetDescriptor)
    :
    assetDescriptor(std::move(assetDescriptor)),
    registry(&registry)
{
}

void AssetPlugin::defineRenderStages(RenderGraph& renderGraph)
{
    renderGraph.insert(resourceUpdateStage);
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
        resourceUpdateStage,
        [reg=parent->registry](vk::CommandBuffer cmdBuf, GlobalUpdateContext& ctx) {
            reg->updateDeviceResources(cmdBuf, ctx.frame());
        }
    );
}

} // namespace trc
