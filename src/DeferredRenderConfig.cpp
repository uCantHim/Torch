#include "DeferredRenderConfig.h"

#include "Window.h"
#include "core/DrawConfiguration.h"

#include "TorchResources.h"
#include "RenderPassDeferred.h"
#include "PipelineDefinitions.h"  // TODO: Enums are here - remove this



trc::DeferredRenderConfig::DeferredRenderConfig(const DeferredRenderCreateInfo& info)
    :
    RenderConfigCrtpBase(info.instance),
    // Passes
    deferredPass(
        info.instance.getDevice(),
        info.window.getSwapchain(),
        info.maxTransparentFragsPerPixel
    ),
    shadowPass(info.instance, uvec2(1, 1)),
    // Descriptors
    globalDataDescriptor(info.window),
    sceneDescriptor(info.window),
    shadowDescriptor(info.window),
    // Asset storage
    assetRegistry(info.assetRegistry),
    // Internal resources
    fullscreenQuadVertexBuffer(
        info.instance.getDevice(),
        std::vector<vec3>{
            vec3(-1, 1, 0), vec3(1, 1, 0), vec3(-1, -1, 0),
            vec3(1, 1, 0), vec3(1, -1, 0), vec3(-1, -1, 0)
        },
        vk::BufferUsageFlagBits::eVertexBuffer
    )
{
    if (info.assetRegistry == nullptr)
    {
        throw std::invalid_argument(
            "Member assetRegistry in DeferredRenderCreateInfo may not be nullptr!"
        );
    }

    // Specify basic graph layout
    graph.first(RenderStageTypes::getDeferred());
    graph.before(RenderStageTypes::getDeferred(), RenderStageTypes::getShadow());

    // Add pass to deferred stage
    graph.addPass(RenderStageTypes::getDeferred(), deferredPass);
}

void trc::DeferredRenderConfig::preDraw(const DrawConfig& draw)
{
    // Add final lighting function to scene
    finalLightingFunc = draw.scene->registerDrawFunction(
        RenderStageTypes::getDeferred(),
        RenderPassDeferred::SubPasses::lighting,
        internal::getFinalLightingPipeline(),
        [&](auto&&, vk::CommandBuffer cmdBuf)
        {
            cmdBuf.bindVertexBuffers(0, *fullscreenQuadVertexBuffer, vk::DeviceSize(0));
            cmdBuf.draw(6, 1, 0, 0);
        }
    );

    globalDataDescriptor.update(*draw.camera);
    sceneDescriptor.update(*draw.scene);
    shadowDescriptor.update(draw.scene->getLightRegistry());
}

void trc::DeferredRenderConfig::postDraw(const DrawConfig& draw)
{
    // Remove fullscreen quad function
    draw.scene->unregisterDrawFunction(finalLightingFunc);
}

auto trc::DeferredRenderConfig::getDeferredRenderPass() const -> const RenderPassDeferred&
{
    return deferredPass;
}

auto trc::DeferredRenderConfig::getCompatibleShadowRenderPass() const -> vk::RenderPass
{
    return *shadowPass;
}

auto trc::DeferredRenderConfig::getGlobalDataDescriptorProvider() const
    -> const DescriptorProviderInterface&
{
    return globalDataDescriptor;
}

auto trc::DeferredRenderConfig::getSceneDescriptorProvider() const
    -> const DescriptorProviderInterface&
{
    return sceneDescriptor.getProvider();
}

auto trc::DeferredRenderConfig::getDeferredPassDescriptorProvider() const
    -> const DescriptorProviderInterface&
{
    return deferredPass.getDescriptorProvider();
}

auto trc::DeferredRenderConfig::getShadowDescriptorProvider() const
    -> const DescriptorProviderInterface&
{
    return shadowDescriptor.getProvider();
}

auto trc::DeferredRenderConfig::getAssetDescriptorProvider() const
    -> const DescriptorProviderInterface&
{
    return assetRegistry->getDescriptorSetProvider();
}

auto trc::DeferredRenderConfig::getAnimationDataDescriptorProvider() const
    -> const DescriptorProviderInterface&
{
    return assetRegistry->getAnimations().getProvider();
}

auto trc::DeferredRenderConfig::getAssets() -> AssetRegistry&
{
    return *assetRegistry;
}

auto trc::DeferredRenderConfig::getAssets() const -> const AssetRegistry&
{
    return *assetRegistry;
}
