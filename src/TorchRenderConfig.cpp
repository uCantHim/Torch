#include "trc/TorchRenderConfig.h"

#include "trc_util/Timer.h"

#include "trc/GBufferPass.h"
#include "trc/TorchImplementation.h"
#include "trc/TorchRenderStages.h"
#include "trc/base/Logging.h"
#include "trc/core/SceneBase.h"
#include "trc/core/Window.h"
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

    // Gui stage
    graph.after(finalLightingRenderStage, guiRenderStage);

    return graph;
}



trc::TorchRenderConfig::TorchRenderConfig(
    const Instance& instance,
    const TorchRenderConfigCreateInfo& info)
    :
    RenderConfig(instance, info.target),
    instance(instance),
    device(instance.getDevice()),
    renderTarget(&info.target),
    mousePosGetter(info.mousePosGetter),
    // Resources
    shadowPool(instance.getDevice(), info.target.getFrameClock(), { .maxShadowMaps=50 }),
    // Passes
    gBuffer(nullptr),
    gBufferPass(nullptr),
    shadowPass(device, info.target.getFrameClock(), { .shadowIndex=0, .resolution=uvec2(1, 1) }),
    // Descriptors
    gBufferDescriptor(device, info.target.getFrameClock().getFrameCount()),
    globalDataDescriptor(device, info.target.getFrameClock().getFrameCount()),
    sceneDescriptor(instance.getDevice()),
    assetDescriptor(info.assetDescriptor)
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
    resourceConfig.defineDescriptor(DescriptorName{ ASSET_DESCRIPTOR },
                                    assetDescriptor->getDescriptorSetLayout());
    resourceConfig.defineDescriptor(DescriptorName{ GLOBAL_DATA_DESCRIPTOR },
                                    globalDataDescriptor.getDescriptorSetLayout());
    resourceConfig.defineDescriptor(DescriptorName{ G_BUFFER_DESCRIPTOR },
                                    gBufferDescriptor.getDescriptorSetLayout());
    resourceConfig.defineDescriptor(DescriptorName{ SCENE_DESCRIPTOR },
                                    sceneDescriptor.getDescriptorSetLayout());
    resourceConfig.defineDescriptor(DescriptorName{ SHADOW_DESCRIPTOR },
                                    shadowPool.getDescriptorSetLayout());

    // Define named render passes
    resourceConfig.addRenderPass(
        RenderPassName{ OPAQUE_G_BUFFER_PASS },
        [&]{ return RenderPassInfo{ *getGBufferRenderPass(), 0 }; }
    );
    resourceConfig.addRenderPass(
        RenderPassName{ TRANSPARENT_G_BUFFER_PASS },
        [&]{ return RenderPassInfo{ *getGBufferRenderPass(), 1 }; }
    );
    resourceConfig.addRenderPass(
        RenderPassName{ SHADOW_PASS },
        [&]{ return RenderPassInfo{ getCompatibleShadowRenderPass(), 0 }; }
    );

    // The final lighting pass wants to create a pipeline layout, so it has
    // to be created after the descriptors have been defined.
    //finalLightingPass = std::make_unique<FinalLightingPass>(
    //    device, info.target, uvec2(0, 0), uvec2(1, 1), *this
    //);
    //renderGraph.addPass(finalLightingRenderStage, *finalLightingPass);

    //renderGraph.addPass(resourceUpdateStage, info.assetRegistry->getUpdatePass());
}

void trc::TorchRenderConfig::perFrameUpdate(const Camera&, const SceneBase& scene)
{
    // Add final lighting function to scene
    //globalDataDescriptor.update(camera);
    sceneDescriptor.update(scene);
    assetDescriptor->update(device);
    shadowPool.update();
}

void trc::TorchRenderConfig::setViewport(ivec2 offset, uvec2 size)
{
    viewportOffset = offset;
    viewportSize = size;
    createGBuffer(size);
    //finalLightingPass->setTargetArea(offset, size);
}

void trc::TorchRenderConfig::setRenderTarget(const RenderTarget& target)
{
    renderTarget = &target;
    //finalLightingPass->setRenderTarget(device, target);
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

auto trc::TorchRenderConfig::getShadowPool() -> ShadowPool&
{
    return shadowPool;
}

auto trc::TorchRenderConfig::getShadowPool() const -> const ShadowPool&
{
    return shadowPool;
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
        auto pos = mousePosGetter() - vec2(viewportOffset);
        pos.y = viewportSize.y - pos.y;
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
    //renderGraph.removePass(gBufferRenderStage, *gBufferPass);
    //if (mouseDepthReader != nullptr)
    //{
    //    renderGraph.removePass(mouseDepthReadStage, *mouseDepthReader);
    //    mouseDepthReader.reset();
    //}
    gBufferPass.reset();
    gBuffer.reset();
    log::info << "GBuffer resources destroyed (" << timer.reset() << " ms)";

    // Create new g-buffer
    gBuffer = std::make_unique<FrameSpecific<GBuffer>>(
        renderTarget->getFrameClock(),
        [this, newSize](ui32) {
            return GBuffer(
                device,
                GBufferCreateInfo{ .size=newSize, .maxTransparentFragsPerPixel=3 }
            );
        }
    );
    log::info << "GBuffer recreated (" << timer.reset() << " ms)";

    // Update g-buffer descriptor
    //gBufferDescriptor.update(device, *gBuffer);
    log::info << "GBuffer descriptor updated (" << timer.reset() << " ms)";

    // Create new renderpasses
    //gBufferPass = std::make_unique<GBufferPass>(device, *gBuffer);
    //renderGraph.addPass(gBufferRenderStage, *gBufferPass);
    //mouseDepthReader = std::make_unique<GBufferDepthReader>(
    //    device,
    //    [this]{ return mousePosGetter() - vec2(viewportOffset); },
    //    *gBuffer
    //);
    //renderGraph.addPass(mouseDepthReadStage, *mouseDepthReader);

    log::info << "Deferred renderpass recreated (" << timer.reset() << " ms)";
}
