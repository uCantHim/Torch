#include "trc/TorchRenderConfig.h"

#include "trc_util/Timer.h"

#include "trc/GBufferPass.h"
#include "trc/Scene.h"
#include "trc/TorchImplementation.h"
#include "trc/TorchRenderStages.h"
#include "trc/base/Logging.h"
#include "trc/core/DrawConfiguration.h"
#include "trc/core/Window.h"
#include "trc/text/Font.h"
#include "trc/ui/torch/GuiIntegration.h"



auto trc::makeTorchRenderGraph() -> RenderGraph
{
    RenderGraph graph;

    // Deferred rendering stages
    graph.first(resourceUpdateStage);
    graph.after(resourceUpdateStage, shadowRenderStage);
    graph.after(shadowRenderStage, gBufferRenderStage);
    graph.after(gBufferRenderStage, mouseDepthReadStage);
    graph.after(mouseDepthReadStage, finalLightingRenderStage);

    // Ray tracing stages
    graph.after(finalLightingRenderStage, rayTracingRenderStage);
    graph.require(rayTracingRenderStage, resourceUpdateStage);
    graph.require(rayTracingRenderStage, finalLightingRenderStage);

    // Gui stage
    graph.after(rayTracingRenderStage, guiRenderStage);

    return graph;
}



trc::TorchRenderConfig::TorchRenderConfig(
    const Window& _window,
    const TorchRenderConfigCreateInfo& info)
    :
    RenderConfigImplHelper(_window.getInstance(), makeTorchRenderGraph()),
    window(_window),
    renderTarget(&info.target),
    enableRayTracing(info.enableRayTracing && _window.getInstance().hasRayTracing()),
    // Passes
    gBuffer(nullptr),
    gBufferPass(nullptr),
    shadowPass(window, { .shadowIndex=0, .resolution=uvec2(1, 1) }),
    // Descriptors
    gBufferDescriptor(_window.getDevice(), _window),  // Don't update the descriptor sets yet!
    globalDataDescriptor(window),
    sceneDescriptor(window.getInstance()),
    assetDescriptor(info.assetDescriptor),
    // Asset storage
    shadowPool(info.shadowPool)
{
    if (info.assetRegistry == nullptr)
    {
        throw std::invalid_argument(
            "Member assetRegistry in DeferredRenderCreateInfo may not be nullptr!"
        );
    }

    // Optionally create ray tracing resources
    if (enableRayTracing)
    {
        tlas = std::make_unique<rt::TLAS>(_window.getInstance(), 5000);
        tlasBuildPass = std::make_unique<TopLevelAccelerationStructureBuildPass>(
            _window.getInstance(),
            *tlas
        );
        renderGraph.addPass(resourceUpdateStage, *tlasBuildPass);
    }

    // Create gbuffer for the first time
    createGBuffer({ 1, 1 });

    // Define named descriptors
    addDescriptor(DescriptorName{ GLOBAL_DATA_DESCRIPTOR }, getGlobalDataDescriptorProvider());
    addDescriptor(DescriptorName{ ASSET_DESCRIPTOR },       getAssetDescriptorProvider());
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
    renderGraph.addPass(finalLightingRenderStage, *finalLightingPass);

    renderGraph.addPass(resourceUpdateStage, info.assetRegistry->getUpdatePass());
}

void trc::TorchRenderConfig::perFrameUpdate(const Camera& camera, const Scene& scene)
{
    // Add final lighting function to scene
    globalDataDescriptor.update(camera);
    sceneDescriptor.update(scene);
    assetDescriptor->update(window.getDevice());
    shadowPool->update();
    if (enableRayTracing) {
        tlasBuildPass->setScene(scene.getComponentInternals());
    }
}

void trc::TorchRenderConfig::setViewport(uvec2 offset, uvec2 size)
{
    assert(finalLightingPass != nullptr);

    viewportOffset = offset;
    viewportSize = size;
    createGBuffer(size);
    finalLightingPass->setTargetArea(offset, size);
}

void trc::TorchRenderConfig::setRenderTarget(const RenderTarget& target)
{
    assert(finalLightingPass != nullptr);

    renderTarget = &target;
    finalLightingPass->setRenderTarget(window.getDevice(), target);
    if (enableRayTracing) {
        rayTracingPass->setRenderTarget(target);
    }
}

void trc::TorchRenderConfig::setClearColor(vec4 color)
{
    if (gBufferPass != nullptr) {
        gBufferPass->setClearColor(color);
    }
}

auto trc::TorchRenderConfig::getGBuffer() -> FrameSpecific<GBuffer>&
{
    return *gBuffer;
}

auto trc::TorchRenderConfig::getGBuffer() const -> const FrameSpecific<GBuffer>&
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
    return *assetDescriptor;
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
    return mouseDepthReader->getMouseDepth();
}

auto trc::TorchRenderConfig::getMousePosAtDepth(const Camera& camera, const float depth) const
    -> vec3
{
    const vec2 mousePos = glm::clamp([this]() -> vec2 {
#ifdef TRC_FLIP_Y_PROJECTION
        auto pos = window.getMousePositionLowerLeft();
        pos.x -= viewportOffset.x;
        pos.y -= window.getWindowSize().y - (viewportOffset.y + viewportSize.y);
        return pos;
#else
        return window.getMousePosition() - vec2(viewportOffset);
#endif
    }(), vec2(0.0f, 0.0f), vec2(viewportSize) - 1.0f);

    return camera.unproject(mousePos, depth, viewportSize);
}

auto trc::TorchRenderConfig::getMouseWorldPos(const Camera& camera) const -> vec3
{
    return getMousePosAtDepth(camera, mouseDepthReader->getMouseDepth());
}

void trc::TorchRenderConfig::createGBuffer(const uvec2 newSize)
{
    if (gBuffer != nullptr && gBuffer->get().getSize() == newSize) {
        return;
    }

    trc::Timer timer;

    // Delete resources
    if (enableRayTracing)
    {
        renderGraph.removePass(rayTracingRenderStage, *rayTracingPass);
        rayTracingPass.reset();
    }
    renderGraph.removePass(gBufferRenderStage, *gBufferPass);
    if (mouseDepthReader != nullptr)
    {
        renderGraph.removePass(mouseDepthReadStage, *mouseDepthReader);
        mouseDepthReader.reset();
    }
    gBufferPass.reset();
    gBuffer.reset();
    log::info << "GBuffer resources destroyed (" << timer.reset() << " ms)";

    // Create new g-buffer
    gBuffer = std::make_unique<FrameSpecific<GBuffer>>(
        window.getSwapchain(),
        [this, newSize](ui32) {
            return GBuffer(
                window.getDevice(),
                GBufferCreateInfo{ .size=newSize, .maxTransparentFragsPerPixel=3 }
            );
        }
    );
    log::info << "GBuffer recreated (" << timer.reset() << " ms)";

    // Update g-buffer descriptor
    gBufferDescriptor.update(window.getDevice(), *gBuffer);
    log::info << "GBuffer descriptor updated (" << timer.reset() << " ms)";

    // Create new renderpasses
    gBufferPass = std::make_unique<GBufferPass>(window.getDevice(), *gBuffer);
    renderGraph.addPass(gBufferRenderStage, *gBufferPass);
    mouseDepthReader = std::make_unique<GBufferDepthReader>(
        window.getDevice(),
        [this]{ return window.getMousePosition() - vec2(viewportOffset); },
        *gBuffer
    );
    renderGraph.addPass(mouseDepthReadStage, *mouseDepthReader);

    if (enableRayTracing)
    {
        FrameSpecific<rt::RayBuffer> rayBuffer{
            window,
            [&](ui32) {
                return trc::rt::RayBuffer(
                    window.getDevice(),
                    { window.getSize(), vk::ImageUsageFlagBits::eStorage }
                );
            }
        };

        assert(renderTarget != nullptr);
        rayTracingPass = std::make_unique<RayTracingPass>(
            window.getInstance(),
            *this,
            *tlas,
            std::move(rayBuffer),
            *renderTarget
        );
        renderGraph.addPass(rayTracingRenderStage, *rayTracingPass);
    }

    log::info << "Deferred renderpass recreated (" << timer.reset() << " ms)";
}
