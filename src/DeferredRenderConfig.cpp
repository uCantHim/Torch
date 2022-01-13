#include "DeferredRenderConfig.h"

#include "core/Window.h"
#include "core/DrawConfiguration.h"
#include "trc_util/Timer.h"
#include "TorchResources.h"
#include "RenderPassDeferred.h"
#include "PipelineDefinitions.h"  // TODO: Enums are here - remove this



auto trc::makeDeferredRenderGraph() -> RenderGraph
{
    RenderGraph graph;

    graph.first(shadowRenderStage);
    graph.after(shadowRenderStage, deferredRenderStage);
    graph.after(deferredRenderStage, finalLightingRenderStage);

    return graph;
}



trc::DeferredRenderConfig::DeferredRenderConfig(
    const Window& _window,
    const DeferredRenderCreateInfo& info)
    :
    RenderConfigCrtpBase(_window.getInstance(), RenderLayout(_window, info.renderGraph)),
    window(_window),
    // Passes
    gBuffer(nullptr),
    deferredPass(nullptr),
    shadowPass(window, { .shadowIndex=0, .resolution=uvec2(1, 1) }),
    // Descriptors
    gBufferDescriptor(_window.getDevice(), _window),  // Don't update the descriptor sets yet!
    globalDataDescriptor(window),
    sceneDescriptor(window),
    fontDataDescriptor(info.assetRegistry->getFonts().getDescriptorSetLayout(), {}),
    // Asset storage
    assetRegistry(info.assetRegistry),
    shadowPool(info.shadowPool)
{
    if (info.assetRegistry == nullptr)
    {
        throw std::invalid_argument(
            "Member assetRegistry in DeferredRenderCreateInfo may not be nullptr!"
        );
    }

    // Create gbuffer for the first time
    createGBuffer({ 1, 1 });

    // Define named descriptors
    addDescriptor(DescriptorName{ GLOBAL_DATA_DESCRIPTOR }, getGlobalDataDescriptorProvider());
    addDescriptor(DescriptorName{ ASSET_DESCRIPTOR },       getAssetDescriptorProvider());
    addDescriptor(DescriptorName{ ANIMATION_DESCRIPTOR },   getAnimationDataDescriptorProvider());
    addDescriptor(DescriptorName{ FONT_DESCRIPTOR },        getFontDescriptorProvider());
    addDescriptor(DescriptorName{ SCENE_DESCRIPTOR },       getSceneDescriptorProvider());
    addDescriptor(DescriptorName{ G_BUFFER_DESCRIPTOR },    getGBufferDescriptorProvider());
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
        RenderPassName{ SHADOW_PASS },
        [&]{ return RenderPassDefinition{ getCompatibleShadowRenderPass(), 0 }; }
    );

    // The final lighting pass wants to create a pipeline layout, so it has
    // to be created after the descriptors have been defined.
    finalLightingPass = std::make_unique<FinalLightingPass>(
        window.getDevice(), info.target, uvec2(0, 0), uvec2(1, 1), *this
    );
    layout.addPass(finalLightingRenderStage, *finalLightingPass);
}

void trc::DeferredRenderConfig::preDraw(const DrawConfig& draw)
{
    // Add final lighting function to scene
    globalDataDescriptor.update(*draw.camera);
    sceneDescriptor.update(*draw.scene);
    shadowPool->update();
}

void trc::DeferredRenderConfig::postDraw(const DrawConfig&)
{
}

void trc::DeferredRenderConfig::setViewport(uvec2 offset, uvec2 size)
{
    assert(finalLightingPass != nullptr);

    createGBuffer(size);
    finalLightingPass->setTargetArea(offset, size);
}

void trc::DeferredRenderConfig::setRenderTarget(const RenderTarget& target)
{
    assert(finalLightingPass != nullptr);

    finalLightingPass->setRenderTarget(window.getDevice(), target);
}

void trc::DeferredRenderConfig::setClearColor(vec4 color)
{
    if (deferredPass != nullptr) {
        deferredPass->setClearColor(color);
    }
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

auto trc::DeferredRenderConfig::getGBufferDescriptorProvider() const
    -> const DescriptorProviderInterface&
{
    return gBufferDescriptor.getProvider();
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

auto trc::DeferredRenderConfig::getMouseWorldPos(const Camera& camera) const -> vec3
{
    return deferredPass->getMousePos(camera);
}

void trc::DeferredRenderConfig::createGBuffer(const uvec2 newSize)
{
    if (gBuffer != nullptr && gBuffer->get().getSize() == newSize) {
        return;
    }

    trc::Timer timer;

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

    // Update g-buffer descriptor
    gBufferDescriptor.update(window.getDevice(), *gBuffer);

    if constexpr (vkb::enableVerboseLogging)
    {
        const float time = timer.reset();
        std::cout << "GBuffer descriptor updated (" << time << " ms)\n";
    }

    // Create new renderpass
    deferredPass = std::make_unique<RenderPassDeferred>(
        window.getDevice(),
        window.getSwapchain(),
        *gBuffer
    );
    layout.addPass(deferredRenderStage, *deferredPass);

    if constexpr (vkb::enableVerboseLogging)
    {
        const float time = timer.reset();
        std::cout << "Deferred renderpass recreated for new swapchain (" << time << " ms)\n";
    }
}
