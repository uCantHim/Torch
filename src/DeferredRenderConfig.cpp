#include "DeferredRenderConfig.h"

#include <vkb/util/Timer.h>

#include "core/Window.h"
#include "core/DrawConfiguration.h"
#include "TorchResources.h"
#include "RenderPassDeferred.h"
#include "PipelineDefinitions.h"  // TODO: Enums are here - remove this



trc::DeferredRenderConfig::DeferredRenderConfig(
    const Window& _window,
    const DeferredRenderCreateInfo& info)
    :
    RenderConfigCrtpBase(_window.getInstance()),
    window(_window),
    // Passes
    deferredPass(new RenderPassDeferred(
        window.getDevice(),
        window.getSwapchain(),
        { window.getSwapchain().getSize(), info.maxTransparentFragsPerPixel }
    )),
    shadowPass(window, { .shadowIndex=0, .resolution=uvec2(1, 1) }),
    // Descriptors
    globalDataDescriptor(window),
    sceneDescriptor(window),
    // Asset storage
    assetRegistry(info.assetRegistry),
    shadowPool(info.shadowPool),
    // Internal resources
    fullscreenQuadVertexBuffer(
        window.getDevice(),
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

    // Initialize descriptor provider for the deferred renderpass
    auto& p = deferredPass->getDescriptorProvider();
    deferredPassDescriptorProvider.setWrappedProvider(p);
    deferredPassDescriptorProvider.setDescLayout(p.getDescriptorSetLayout());

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
            { window.getSwapchain().getSize(), info.maxTransparentFragsPerPixel }
        ));
        graph.addPass(RenderStageTypes::getDeferred(), *deferredPass);
        if constexpr (vkb::enableVerboseLogging)
        {
            const float time = timer.reset();
            std::cout << "Deferred renderpass recreated for new swapchain"
                << " (" << time << " ms)\n";
        }

        auto& p = deferredPass->getDescriptorProvider();
        deferredPassDescriptorProvider.setWrappedProvider(p);
        deferredPassDescriptorProvider.setDescLayout(p.getDescriptorSetLayout());
    }).makeUnique();
}

void trc::DeferredRenderConfig::preDraw(const DrawConfig& draw)
{
    // Add final lighting function to scene
    finalLightingFunc = draw.scene->registerDrawFunction(
        RenderStageTypes::getDeferred(),
        RenderPassDeferred::SubPasses::lighting,
        getFinalLightingPipeline(),
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

auto trc::DeferredRenderConfig::getGBuffer() -> vkb::FrameSpecific<GBuffer>&
{
    return deferredPass->getGBuffer();
}

auto trc::DeferredRenderConfig::getGBuffer() const -> const vkb::FrameSpecific<GBuffer>&
{
    return deferredPass->getGBuffer();
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
    return deferredPassDescriptorProvider;
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
