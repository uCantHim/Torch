#include "Renderer.h"

#include <vkb/util/Timer.h>

#include "PipelineDefinitions.h"
#include "PipelineRegistry.h"
#include "AssetRegistry.h"
#include "Particle.h" // For particle pipeline creation
#include "Scene.h"



/////////////////////////
//      Renderer       //
/////////////////////////

trc::Renderer::Renderer(RendererCreateInfo info)
    :
    // Create the deferred render pass
    defaultDeferredPass(RenderPass::createAtNextIndex<RenderPassDeferred>(
        vkb::getSwapchain(), info.maxTransparentFragsPerPixel
    ).first),
    fullscreenQuadVertexBuffer(
        std::vector<vec3>{
            vec3(-1, 1, 0), vec3(1, 1, 0), vec3(-1, -1, 0),
            vec3(1, 1, 0), vec3(1, -1, 0), vec3(-1, -1, 0)
        },
        vk::BufferUsageFlagBits::eVertexBuffer
    )
{
    // Specify basic graph layout
    renderGraph.first(RenderStageTypes::getDeferred());
    renderGraph.before(RenderStageTypes::getDeferred(), RenderStageTypes::getShadow());

    // Add pass to deferred stage
    renderGraph.addPass(RenderStageTypes::getDeferred(), defaultDeferredPass);

    createSemaphores();

    // Pre recreate, finish rendering
    preRecreateListener = vkb::EventHandler<vkb::PreSwapchainRecreateEvent>::addListener(
        [this](const auto&) {
            waitForAllFrames();
        }
    );
    // Post recreate, create the required resources
    postRecreateListener = vkb::EventHandler<vkb::SwapchainRecreateEvent>::addListener(
        [this, maxFrags=info.maxTransparentFragsPerPixel](const auto&) {
            // Completely recreate the deferred renderpass
            RenderPass::replace<RenderPassDeferred>(
                defaultDeferredPass,
                vkb::getSwapchain(), maxFrags
            );
            PipelineRegistry::recreateAll();
        }
    );

    PipelineRegistry::registerPipeline([this]() { internal::makeAllDrawablePipelines(*this); });
    PipelineRegistry::registerPipeline([this]() { internal::makeFinalLightingPipeline(*this); });
    PipelineRegistry::registerPipeline([this]() {
        internal::makeParticleDrawPipeline(*this);
        internal::makeParticleShadowPipeline(*this);
    });
    // Create all pipelines for the first time
    PipelineRegistry::recreateAll();
}

trc::Renderer::~Renderer()
{
    waitForAllFrames();
}

void trc::Renderer::drawFrame(Scene& scene, const Camera& camera)
{
    auto& device = vkb::getDevice();
    auto& swapchain = vkb::getSwapchain();

    size_t size = 0;
    renderGraph.foreachStage([&](auto&&, auto&&) { size++; });
    while (size > commandCollectors.size()) {
        commandCollectors.emplace_back();
    }

    // Update
    scene.update();
    sceneDescriptorProvider.setWrappedProvider(scene.getDescriptor().getProvider());
    shadowDescriptorProvider.setWrappedProvider(
        scene.getLightRegistry().getDescriptor().getProvider()
    );
    globalDataDescriptor.updateCameraMatrices(camera);
    globalDataDescriptor.updateSwapchainData(swapchain);

    // Add final lighting function to scene
    auto finalLightingFunc = scene.registerDrawFunction(
        RenderStageTypes::getDeferred(),
        internal::DeferredSubPasses::eLightingPass,
        internal::Pipelines::eFinalLighting,
        [&](auto&&, vk::CommandBuffer cmdBuf)
        {
            cmdBuf.bindVertexBuffers(0, *fullscreenQuadVertexBuffer, vk::DeviceSize(0));
            cmdBuf.draw(6, 1, 0, 0);
        }
    );

    // Acquire image
    auto fenceResult = device->waitForFences(**frameInFlightFences, true, UINT64_MAX);
    assert(fenceResult == vk::Result::eSuccess);
    device->resetFences(**frameInFlightFences);
    auto image = swapchain.acquireImage(**imageAcquireSemaphores);

    // Collect commands
    std::vector<vk::CommandBuffer> cmdBufs;
    int i{ 0 };
    renderGraph.foreachStage(
        [&, this](RenderStageType::ID stage, const std::vector<RenderPass::ID>& passes)
        {
            if (stage == RenderStageTypes::getShadow())
            {
                auto mergedPasses = scene.getLightRegistry().getShadowRenderStage();
                mergedPasses.insert(mergedPasses.end(), passes.begin(), passes.end());

                cmdBufs.push_back(commandCollectors.at(i).recordScene(
                    scene, stage,
                    mergedPasses
                ));
            }
            else {
                cmdBufs.push_back(commandCollectors.at(i).recordScene(scene, stage, passes));
            }

            i++;
        }
    );

    // Remove fullscreen quad function
    scene.unregisterDrawFunction(finalLightingFunc);

    // Submit command buffers
    auto graphicsQueue = vkb::getDevice().getQueue(vkb::QueueType::graphics);
    vk::PipelineStageFlags waitStage = vk::PipelineStageFlagBits::eVertexInput;
    graphicsQueue.submit(
        vk::SubmitInfo(
            **imageAcquireSemaphores,
            waitStage,
            cmdBufs,
            **renderFinishedSemaphores
        ),
        **frameInFlightFences
    );

    // Present frame
    auto presentQueue = vkb::getDevice().getQueue(vkb::QueueType::presentation);
    swapchain.presentImage(image, presentQueue, { **renderFinishedSemaphores });
}

auto trc::Renderer::getRenderGraph() noexcept -> RenderGraph&
{
    return renderGraph;
}

auto trc::Renderer::getRenderGraph() const noexcept -> const RenderGraph&
{
    return renderGraph;
}

auto trc::Renderer::getDeferredRenderPass() const noexcept -> const RenderPassDeferred&
{
    return static_cast<RenderPassDeferred&>(RenderPass::at(defaultDeferredPass));
}

auto trc::Renderer::getGlobalDataDescriptor() const noexcept -> const GlobalRenderDataDescriptor&
{
    return globalDataDescriptor;
}

auto trc::Renderer::getGlobalDataDescriptorProvider() const noexcept
    -> const DescriptorProviderInterface&
{
    return globalDataDescriptor.getProvider();
}

auto trc::Renderer::getSceneDescriptorProvider() const noexcept
    -> const DescriptorProviderInterface&
{
    return sceneDescriptorProvider;
}

auto trc::Renderer::getShadowDescriptorProvider() const noexcept
    -> const DescriptorProviderInterface&
{
    return shadowDescriptorProvider;
}

auto trc::Renderer::getMouseWorldPos(const Camera& camera) -> vec3
{
    return getMouseWorldPosAtDepth(camera, getDeferredRenderPass().getMouseDepth());
}

auto trc::Renderer::getMouseWorldPosAtDepth(const Camera& camera, const float depth) -> vec3
{
    assert(depth >= 0.0f && depth <= 1.0f);

    const auto windowSize = vkb::getSwapchain().getImageExtent();
    const vec2 mousePos = vkb::getSwapchain().getMousePosition();

    return glm::unProject(
        vec3(mousePos, depth),
        camera.getViewMatrix(),
        camera.getProjectionMatrix(),
        vec4(0.0f, 0.0f, windowSize.width, windowSize.height)
    );
}

void trc::Renderer::createSemaphores()
{
    imageAcquireSemaphores = { [](ui32) { return vkb::getDevice()->createSemaphoreUnique({}); }};
    renderFinishedSemaphores = { [](ui32) { return vkb::getDevice()->createSemaphoreUnique({}); }};
    frameInFlightFences = { [](ui32) {
        return vkb::getDevice()->createFenceUnique(
            { vk::FenceCreateFlagBits::eSignaled }
        );
    }};
}

void trc::Renderer::waitForAllFrames(ui64 timeoutNs)
{
    std::vector<vk::Fence> fences;
    for (auto& fence : frameInFlightFences) {
        fences.push_back(*fence);
    }
    auto result = vkb::VulkanBase::getDevice()->waitForFences(fences, true, timeoutNs);
    if (result == vk::Result::eTimeout) {
        std::cout << "Timeout in Renderer::waitForAllFrames!\n";
    }
}



///////////////////////////////////////////
//      Descriptor provider wrapper      //
///////////////////////////////////////////

trc::Renderer::DescriptorProviderWrapper::DescriptorProviderWrapper(
    vk::DescriptorSetLayout staticLayout)
    :
    descLayout(staticLayout)
{
}

auto trc::Renderer::DescriptorProviderWrapper::getDescriptorSet() const noexcept
    -> vk::DescriptorSet
{
    if (provider != nullptr) {
        return provider->getDescriptorSet();
    }
    return {};
}

auto trc::Renderer::DescriptorProviderWrapper::getDescriptorSetLayout() const noexcept
    -> vk::DescriptorSetLayout
{
    return descLayout;
}

void trc::Renderer::DescriptorProviderWrapper::bindDescriptorSet(
    vk::CommandBuffer cmdBuf,
    vk::PipelineBindPoint bindPoint,
    vk::PipelineLayout pipelineLayout,
    ui32 setIndex) const
{
    if (provider != nullptr) {
        provider->bindDescriptorSet(cmdBuf, bindPoint, pipelineLayout, setIndex);
    }
}

void trc::Renderer::DescriptorProviderWrapper::setWrappedProvider(
    const DescriptorProviderInterface& wrapped) noexcept
{
    provider = &wrapped;
}
