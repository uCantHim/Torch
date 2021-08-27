#include "DeferredRenderConfig.h"

#include <vkb/util/Timer.h>

#include "core/Window.h"
#include "core/DrawConfiguration.h"
#include "TorchResources.h"
#include "RenderPassDeferred.h"
#include "PipelineDefinitions.h"  // TODO: Enums are here - remove this



trc::DeferredRenderConfig::DeferredRenderConfig(const DeferredRenderCreateInfo& info)
    :
    RenderConfigCrtpBase(info.instance),
    window(info.window),
    // Passes
    deferredPass(new RenderPassDeferred(
        info.instance.getDevice(),
        info.window.getSwapchain(),
        info.maxTransparentFragsPerPixel
    )),
    shadowPass(info.window, { .shadowIndex=0, .resolution=uvec2(1, 1) }),
    // Descriptors
    globalDataDescriptor(info.window),
    sceneDescriptor(info.window),
    // Asset storage
    assetRegistry(info.assetRegistry),
    shadowPool(info.shadowPool),
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
    graph.addPass(RenderStageTypes::getDeferred(), *deferredPass);

    swapchainRecreateListener = vkb::on<vkb::SwapchainRecreateEvent>([this, info](auto e) {
        if (e.swapchain != &window.getSwapchain()) return;

        vkb::Timer timer;

        graph.removePass(RenderStageTypes::getDeferred(), *deferredPass);
        deferredPass.reset(new RenderPassDeferred(
            window.getDevice(),
            window.getSwapchain(),
            info.maxTransparentFragsPerPixel
        ));
        graph.addPass(RenderStageTypes::getDeferred(), *deferredPass);
        if constexpr (vkb::enableVerboseLogging)
        {
            std::cout << "Deferred renderpass recreated for new swapchain"
                << " (" << timer.reset() << " ms)\n";
        }

        getPipelineStorage().recreateAll();
        if constexpr (vkb::enableVerboseLogging)
        {
            std::cout << "All pipelines recreated for new deferred renderpass"
                << " (" << timer.reset() << " ms)\n";
        }
    }).makeUnique();
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
}

void trc::DeferredRenderConfig::postDraw(const DrawConfig& draw)
{
    // Remove fullscreen quad function
    draw.scene->unregisterDrawFunction(finalLightingFunc);
}

auto trc::DeferredRenderConfig::getDeferredRenderPass() const -> const RenderPassDeferred&
{
    return *deferredPass;
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
    return deferredPass->getDescriptorProvider();
}

auto trc::DeferredRenderConfig::getShadowDescriptorProvider() const
    -> const DescriptorProviderInterface&
{
    return shadowPool->getProvider();
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

auto trc::DeferredRenderConfig::getShadowPool() -> ShadowPool&
{
    return *shadowPool;
}

auto trc::DeferredRenderConfig::getShadowPool() const -> const ShadowPool&
{
    return *shadowPool;
}
