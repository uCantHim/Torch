#include "DeferredRenderConfig.h"

#include <vkb/util/Timer.h>

#include "core/Window.h"
#include "core/DrawConfiguration.h"
#include "TorchResources.h"
#include "RenderPassDeferred.h"
#include "PipelineDefinitions.h"  // TODO: Enums are here - remove this



auto trc::makeDeferredRenderGraph() -> RenderGraph
{
    RenderGraph graph;

    graph.first(shadowRenderStage);
    graph.after(shadowRenderStage, deferredRenderStage);

    return graph;
}



trc::DeferredRenderConfig::DeferredRenderConfig(
    const Window& _window,
    const RenderGraph& graph,
    const DeferredRenderCreateInfo& info)
    :
    DeferredRenderConfig(_window, RenderLayout(_window, graph), info)
{
}

trc::DeferredRenderConfig::DeferredRenderConfig(
    const Window& _window,
    RenderLayout _layout,
    const DeferredRenderCreateInfo& info)
    :
    RenderConfigCrtpBase(_window.getInstance(), std::move(_layout)),
    window(_window),
    // Passes
    gBuffer(nullptr),
    deferredPass(nullptr),
    shadowPass(window, { .shadowIndex=0, .resolution=uvec2(1, 1) }),
    // Descriptors
    globalDataDescriptor(window),
    sceneDescriptor(window),
    fontDataDescriptor(info.assetRegistry->getFonts().getDescriptorSetLayout(), {}),
    // Asset storage
    assetRegistry(info.assetRegistry),
    shadowPool(info.shadowPool),
    // Internal resources
    fullscreenQuadVertexBuffer(
        window.getDevice(),
        std::vector<vec3>{
            vec3(-1, 1, 0), vec3(-1, -1, 0), vec3(1, 1, 0),
            vec3(1, 1, 0), vec3(-1, -1, 0), vec3(1, -1, 0)
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

    swapchainRecreateListener = vkb::on<vkb::SwapchainRecreateEvent>([this](auto e) {
        if (e.swapchain != &window.getSwapchain()) return;
        resizeGBuffer(window.getSwapchain().getSize());
    }).makeUnique();

    resizeGBuffer(window.getSwapchain().getSize());

    // Define named descriptors
    addDescriptor(DescriptorName{ GLOBAL_DATA_DESCRIPTOR }, getGlobalDataDescriptorProvider());
    addDescriptor(DescriptorName{ ASSET_DESCRIPTOR },       getAssetDescriptorProvider());
    addDescriptor(DescriptorName{ ANIMATION_DESCRIPTOR },   getAnimationDataDescriptorProvider());
    addDescriptor(DescriptorName{ FONT_DESCRIPTOR },        getFontDescriptorProvider());
    addDescriptor(DescriptorName{ SCENE_DESCRIPTOR },       getSceneDescriptorProvider());
    addDescriptor(DescriptorName{ G_BUFFER_DESCRIPTOR },    getDeferredPassDescriptorProvider());
    addDescriptor(DescriptorName{ SHADOW_DESCRIPTOR },      getShadowDescriptorProvider());

    // Define named render passes
    addRenderPass(
        RenderPassName{ OPAQUE_G_BUFFER_PASS },
        [&]{ return RenderPassDefinition{ *getDeferredRenderPass(), 0 }; }
    );
    addRenderPass(
        RenderPassName{ TRANSPARENT_G_BUFFER_PASS },
        [&]{ return RenderPassDefinition{ *getDeferredRenderPass(), 1 }; }
    );
    addRenderPass(
        RenderPassName{ FINAL_LIGHTING_PASS },
        [&]{ return RenderPassDefinition{ *getDeferredRenderPass(), 2 }; }
    );
    addRenderPass(
        RenderPassName{ SHADOW_PASS },
        [&]{ return RenderPassDefinition{ getCompatibleShadowRenderPass(), 0 }; }
    );
}

void trc::DeferredRenderConfig::preDraw(const DrawConfig& draw)
{
    // Add final lighting function to scene
    finalLightingFunc = draw.scene->registerDrawFunction(
        deferredRenderStage,
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
    shadowPool->update();
}

void trc::DeferredRenderConfig::postDraw(const DrawConfig& draw)
{
    // Remove fullscreen quad function
    draw.scene->unregisterDrawFunction(finalLightingFunc);
}

auto trc::DeferredRenderConfig::getGBuffer() -> vkb::FrameSpecific<GBuffer>&
{
    return *gBuffer;
}

auto trc::DeferredRenderConfig::getGBuffer() const -> const vkb::FrameSpecific<GBuffer>&
{
    return *gBuffer;
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

auto trc::DeferredRenderConfig::getFontDescriptorProvider() const
    -> const DescriptorProviderInterface&
{
    return fontDataDescriptor;
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

void trc::DeferredRenderConfig::resizeGBuffer(uvec2 newSize)
{
    vkb::Timer timer;

    // Delete resources
    layout.removePass(deferredRenderStage, *deferredPass);
    deferredPass.reset();
    gBuffer.reset();

    // Create new g-buffer
    gBuffer = std::make_unique<vkb::FrameSpecific<GBuffer>>(
        window.getSwapchain(),
        [this, newSize](ui32) {
            return GBuffer(
                window.getDevice(),
                GBufferCreateInfo{ .size=newSize, .maxTransparentFragsPerPixel=3 }
            );
        }
    );
    if constexpr (vkb::enableVerboseLogging)
    {
        const float time = timer.reset();
        std::cout << "GBuffer recreated for new swapchain (" << time << " ms)\n";
    }

    // Create new renderpass
    deferredPass.reset(new RenderPassDeferred(
        window.getDevice(),
        window.getSwapchain(),
        *gBuffer
    ));
    layout.addPass(deferredRenderStage, *deferredPass);

    auto& p = deferredPass->getDescriptorProvider();
    deferredPassDescriptorProvider.setWrappedProvider(p);
    deferredPassDescriptorProvider.setDescLayout(p.getDescriptorSetLayout());

    if constexpr (vkb::enableVerboseLogging)
    {
        const float time = timer.reset();
        std::cout << "Deferred renderpass recreated for new swapchain (" << time << " ms)\n";
    }
}
