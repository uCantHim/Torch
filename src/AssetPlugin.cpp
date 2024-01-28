#include "trc/AssetPlugin.h"

#include "trc/AssetDescriptor.h"
#include "trc/TorchRenderStages.h"
#include "trc/assets/AssetRegistry.h"
#include "trc/core/Frame.h"
#include "trc/core/RenderGraph.h"
#include "trc/core/ResourceConfig.h"
#include "trc/core/Task.h"



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

void AssetPlugin::registerRenderStages(RenderGraph& renderGraph)
{
    renderGraph.first(resourceUpdateStage);
}

void AssetPlugin::defineResources(ResourceConfig& config)
{
    config.defineDescriptor(DescriptorName{ ASSET_DESCRIPTOR },
                            assetDescriptor->getDescriptorSetLayout());
}

void AssetPlugin::registerSceneModules(SceneBase& /*scene*/)
{
}

auto AssetPlugin::createDrawConfig(const Device& /*device*/, Viewport /*renderTarget*/)
    -> u_ptr<DrawConfig>
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

void AssetPlugin::UpdateConfig::update(const Device& device, SceneBase&, const Camera&)
{
    parent->assetDescriptor->update(device);
}

void AssetPlugin::UpdateConfig::createTasks(SceneBase& /*scene*/, TaskQueue& taskQueue)
{
    taskQueue.spawnTask(
        resourceUpdateStage,
        makeTask([reg=parent->registry](vk::CommandBuffer cmdBuf, TaskEnvironment& env) {
            reg->updateDeviceResources(cmdBuf, *env.frame);
        })
    );
}

} // namespace trc
