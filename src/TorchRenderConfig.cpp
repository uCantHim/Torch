#include "TorchRenderConfig.h"

#include "core/Window.h"
#include "core/DrawConfiguration.h"
#include "trc_util/Timer.h"
#include "TorchResources.h"
#include "GBufferPass.h"
#include "PipelineDefinitions.h"  // TODO: Enums are here - remove this



auto trc::makeDeferredRenderGraph() -> RenderGraph
{
    RenderGraph graph;

    graph.first(shadowRenderStage);
    graph.after(shadowRenderStage, gBufferRenderStage);
    graph.after(gBufferRenderStage, finalLightingRenderStage);

    return graph;
}



trc::TorchRenderConfig::TorchRenderConfig(
    const Window& _window,
    const TorchRenderConfigCreateInfo& info)
    :
    RenderConfigCrtpBase(_window.getInstance(), RenderLayout(_window, info.renderGraph)),
    window(_window),
    // Passes
    gBuffer(nullptr),
    gBufferPass(nullptr),
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
        [&]{ return RenderPassDefinition{ *getGBufferRenderPass(), 0 }; }
    );
    addRenderPass(
        RenderPassName{ TRANSPARENT_G_BUFFER_PASS },
        [&]{ return RenderPassDefinition{ *getGBufferRenderPass(), 1 }; }
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

void trc::TorchRenderConfig::preDraw(const DrawConfig& draw)
{
    // Add final lighting function to scene
    globalDataDescriptor.update(*draw.camera);
    sceneDescriptor.update(*draw.scene);
    shadowPool->update();
}

void trc::TorchRenderConfig::postDraw(const DrawConfig&)
{
}

void trc::TorchRenderConfig::setViewport(uvec2 offset, uvec2 size)
{
    assert(finalLightingPass != nullptr);

    createGBuffer(size);
    finalLightingPass->setTargetArea(offset, size);
}

void trc::TorchRenderConfig::setRenderTarget(const RenderTarget& target)
{
    assert(finalLightingPass != nullptr);

    finalLightingPass->setRenderTarget(window.getDevice(), target);
}

void trc::TorchRenderConfig::setClearColor(vec4 color)
{
    if (gBufferPass != nullptr) {
        gBufferPass->setClearColor(color);
    }
}

auto trc::TorchRenderConfig::getGBuffer() -> vkb::FrameSpecific<GBuffer>&
{
    return *gBuffer;
}

auto trc::TorchRenderConfig::getGBuffer() const -> const vkb::FrameSpecific<GBuffer>&
{
    return *gBuffer;
}

auto trc::TorchRenderConfig::getGBufferRenderPass() const -> const GBufferPass&
{
    return *gBufferPass;
}

auto trc::TorchRenderConfig::getCompatibleShadowRenderPass() const -> vk::RenderPass
{
    return *shadowPass;
}

auto trc::TorchRenderConfig::getGlobalDataDescriptorProvider() const
    -> const DescriptorProviderInterface&
{
    return globalDataDescriptor;
}

auto trc::TorchRenderConfig::getSceneDescriptorProvider() const
    -> const DescriptorProviderInterface&
{
    return sceneDescriptor.getProvider();
}

auto trc::TorchRenderConfig::getGBufferDescriptorProvider() const
    -> const DescriptorProviderInterface&
{
    return gBufferDescriptor.getProvider();
}

auto trc::TorchRenderConfig::getShadowDescriptorProvider() const
    -> const DescriptorProviderInterface&
{
    return shadowPool->getProvider();
}

auto trc::TorchRenderConfig::getAssetDescriptorProvider() const
    -> const DescriptorProviderInterface&
{
    return assetRegistry->getDescriptorSetProvider();
}

auto trc::TorchRenderConfig::getFontDescriptorProvider() const
    -> const DescriptorProviderInterface&
{
    return fontDataDescriptor;
}

auto trc::TorchRenderConfig::getAnimationDataDescriptorProvider() const
    -> const DescriptorProviderInterface&
{
    return assetRegistry->getAnimations().getProvider();
}

auto trc::TorchRenderConfig::getAssets() -> AssetRegistry&
{
    return *assetRegistry;
}

auto trc::TorchRenderConfig::getAssets() const -> const AssetRegistry&
{
    return *assetRegistry;
}

auto trc::TorchRenderConfig::getShadowPool() -> ShadowPool&
{
    return *shadowPool;
}

auto trc::TorchRenderConfig::getShadowPool() const -> const ShadowPool&
{
    return *shadowPool;
}

auto trc::TorchRenderConfig::getMouseDepth() const -> float
{
    return gBufferPass->getMouseDepth();
}

auto trc::TorchRenderConfig::getMousePosAtDepth(const Camera& camera, const float depth) const
    -> vec3
{
    const vec2 mousePos = glm::clamp([this]() -> vec2 {
#ifdef TRC_FLIP_Y_PROJECTION
        return window.getMousePositionLowerLeft();
#else
        return window.getMousePosition();
#endif
    }(), vec2(0.0f, 0.0f), vec2(window.getSize()) - 1.0f);

    return camera.unproject(mousePos, depth, window.getSize());
}

auto trc::TorchRenderConfig::getMouseWorldPos(const Camera& camera) const -> vec3
{
    return getMousePosAtDepth(camera, gBufferPass->getMouseDepth());
}

void trc::TorchRenderConfig::createGBuffer(const uvec2 newSize)
{
    if (gBuffer != nullptr && gBuffer->get().getSize() == newSize) {
        return;
    }

    trc::Timer timer;

    // Delete resources
    layout.removePass(gBufferRenderStage, *gBufferPass);
    gBufferPass.reset();
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
    gBufferPass = std::make_unique<GBufferPass>(
        window.getDevice(),
        window.getSwapchain(),
        *gBuffer
    );
    layout.addPass(gBufferRenderStage, *gBufferPass);

    if constexpr (vkb::enableVerboseLogging)
    {
        const float time = timer.reset();
        std::cout << "Deferred renderpass recreated for new swapchain (" << time << " ms)\n";
    }
}
