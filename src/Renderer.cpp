#include "Renderer.h"

#include <vkb/util/Timer.h>
#include <vkb/event/EventHandler.h>
#include <vkb/event/WindowEvents.h>

#include "PipelineDefinitions.h"
#include "PipelineRegistry.h"
#include "AssetRegistry.h"
#include "Particle.h" // For particle pipeline creation
#include "Scene.h"



void trc::terminate()
{
    vkb::getDevice()->waitIdle();

    AssetRegistry::reset();
    RenderPass::destroyAll();
    Pipeline::destroyAll();
    RenderStage::destroyAll();
    vkb::vulkanTerminate();
}



/////////////////////////
//      Renderer       //
/////////////////////////

vkb::StaticInit trc::Renderer::_init{
    []() {
        RenderStage::create<DeferredStage>(internal::RenderStages::eDeferred);
        RenderStage::create<ShadowStage>(internal::RenderStages::eShadow);
    },
    []() {}
};

trc::Renderer::Renderer()
    :
    deferredPassId(RenderPass::createAtNextIndex<RenderPassDeferred>(vkb::getSwapchain(), 3).first),
    fullscreenQuadVertexBuffer(
        std::vector<vec3>{
            vec3(-1, 1, 0), vec3(1, 1, 0), vec3(-1, -1, 0),
            vec3(1, 1, 0), vec3(1, -1, 0), vec3(-1, -1, 0)
        },
        vk::BufferUsageFlagBits::eVertexBuffer
    )
{
    createSemaphores();

    // Pre recreate, finish rendering
    preRecreateListener = vkb::EventHandler<vkb::PreSwapchainRecreateEvent>::addListener(
        [this](const auto&) {
            waitForAllFrames();
        }
    );
    // Post recreate, create the required resources
    postRecreateListener = vkb::EventHandler<vkb::SwapchainRecreateEvent>::addListener(
        [this](const auto&) {
            // Completely recreate the deferred renderpass
            RenderPassDeferred::replace<RenderPassDeferred>(deferredPassId, vkb::getSwapchain(), 4);
            PipelineRegistry::recreateAll();
        }
    );

    // Add default stages
    addStage(internal::RenderStages::eDeferred, 3);
    addStage(internal::RenderStages::eShadow, 1);

    // TODO: Change this after the first phase of the rework because it's crap
    // Add deferred pass the the deferred stage
    RenderStage::at(internal::RenderStages::eDeferred).addRenderPass(deferredPassId);

    PipelineRegistry::registerPipeline([this]() { internal::makeAllDrawablePipelines(*this); });
    PipelineRegistry::registerPipeline([this]() { internal::makeFinalLightingPipeline(*this); });
    PipelineRegistry::registerPipeline([this]() {
        internal::makeParticleDrawPipeline(*this);
        internal::makeParticleShadowPipeline(*this);
    });
    // Create all pipelines for the first time
    PipelineRegistry::recreateAll();
}

void trc::Renderer::drawFrame(Scene& scene, const Camera& camera)
{
    auto& device = vkb::getDevice();
    auto& swapchain = vkb::getSwapchain();

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
        internal::RenderStages::eDeferred,
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
    for (ui32 i = 0; const auto& [stage, _] : renderStages)
    {
        cmdBufs.push_back(commandCollectors[i].recordScene(scene, stage));
        i++;
    }

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

auto trc::Renderer::getDefaultDeferredStageId() const noexcept -> RenderStage::ID
{
    // TODO
    return internal::RenderStages::eDeferred;
}

auto trc::Renderer::getDefaultDeferredStage() const noexcept -> DeferredStage&
{
    return static_cast<DeferredStage&>(RenderStage::at(getDefaultDeferredStageId()));
}

auto trc::Renderer::getDefaultShadowStageId() const noexcept -> RenderStage::ID
{
    // TODO
    return internal::RenderStages::eShadow;
}

auto trc::Renderer::getDefaultShadowStage() const noexcept -> ShadowStage&
{
    return static_cast<ShadowStage&>(RenderStage::at(getDefaultShadowStageId()));
}

void trc::Renderer::addStage(RenderStage::ID stage, ui32 priority)
{
    auto it = renderStages.begin();
    while (it != renderStages.end() && it->second < priority) it++;

    renderStages.insert(it, { stage, priority });
    commandCollectors.emplace_back();
}

auto trc::Renderer::getDeferredRenderPassId() const noexcept -> RenderPass::ID
{
    return deferredPassId;
}

auto trc::Renderer::getDeferredRenderPass() const noexcept -> const RenderPassDeferred&
{
    auto result = dynamic_cast<RenderPassDeferred*>(&RenderPass::at(deferredPassId));
    assert(result != nullptr);

    return *result;
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
