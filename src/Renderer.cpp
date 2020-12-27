#include "Renderer.h"

#include <vkb/util/Timer.h>

#include "PipelineDefinitions.h"
#include "PipelineRegistry.h"
#include "AssetRegistry.h"
#include "Scene.h"



/////////////////////////////////////
//      Static Renderer data       //
/////////////////////////////////////

auto trc::SharedRendererData::getGlobalDataDescriptorProvider() noexcept
    -> const DescriptorProviderInterface&
{
    return globalRenderDataProvider;
}

auto trc::SharedRendererData::getDeferredPassDescriptorProvider() noexcept
    -> const DescriptorProviderInterface&
{
    return deferredDescriptorProvider;
}

auto trc::SharedRendererData::getShadowDescriptorProvider() noexcept
    -> const DescriptorProviderInterface&
{
    return shadowDescriptorProvider;
}

auto trc::SharedRendererData::getSceneDescriptorProvider() noexcept
    -> const DescriptorProviderInterface&
{
    return sceneDescriptorProvider;
}

void trc::SharedRendererData::init(
    const DescriptorProviderInterface& globalDataDescriptor,
    const DescriptorProviderInterface& deferredDescriptor)
{
    std::unique_lock lock{ renderLock };
    globalRenderDataProvider = globalDataDescriptor;
    deferredDescriptorProvider = deferredDescriptor;

    globalRenderDataProvider.setDescLayout(globalDataDescriptor.getDescriptorSetLayout());
    deferredDescriptorProvider.setDescLayout(deferredDescriptor.getDescriptorSetLayout());
    shadowDescriptorProvider.setDescLayout(ShadowDescriptor::getDescLayout());
    sceneDescriptorProvider.setDescLayout(SceneDescriptor::getDescLayout());
}

auto trc::SharedRendererData::beginRender(
    const DescriptorProviderInterface& globalDataDescriptor,
    const DescriptorProviderInterface& deferredDescriptor,
    const DescriptorProviderInterface& shadowDescriptor,
    const DescriptorProviderInterface& sceneDescriptor)
    -> std::unique_lock<std::mutex>
{
    std::unique_lock lock{ renderLock };
    globalRenderDataProvider = globalDataDescriptor;
    deferredDescriptorProvider = deferredDescriptor;
    shadowDescriptorProvider = shadowDescriptor;
    sceneDescriptorProvider = sceneDescriptor;

    return lock;
}

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
    [[maybe_unused]]
    static bool _sharedDataInit = [this] {
        // Initialize shared data
        SharedRendererData::init(
            globalDataDescriptor.getProvider(),
            getDeferredRenderPass().getDescriptorProvider()
        );
        return true;
    }();

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
            auto& newPass = RenderPass::replace<RenderPassDeferred>(
                defaultDeferredPass,
                vkb::getSwapchain(), maxFrags
            );

            // Update descriptors before recreating pipelines
            SharedRendererData::deferredDescriptorProvider = newPass.getDescriptorProvider();
            SharedRendererData::globalRenderDataProvider = globalDataDescriptor.getProvider();

            PipelineRegistry::recreateAll();
        }
    );
}

trc::Renderer::~Renderer()
{
    waitForAllFrames();
}

void trc::Renderer::drawFrame(Scene& scene, const Camera& camera)
{
    auto& device = vkb::getDevice();
    auto& swapchain = vkb::getSwapchain();

    // Ensure that enough command collectors are available
    while (renderGraph.size() > commandCollectors.size()) {
        commandCollectors.emplace_back();
    }

    // Synchronize among multiple Renderer instances
    // This is a lazy solution, but I finally wanted to continue living my life.
    auto interRendererLock = SharedRendererData::beginRender(
        globalDataDescriptor.getProvider(),
        getDeferredRenderPass().getDescriptorProvider(),
        scene.getLightRegistry().getDescriptor().getProvider(),
        scene.getDescriptor().getProvider()
    );

    // Update
    scene.update();
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
