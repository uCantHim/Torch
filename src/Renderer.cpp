#include "Renderer.h"

#include <vkb/util/Timer.h>
#include <vkb/event/EventHandler.h>
#include <vkb/event/WindowEvents.h>

#include "utils/Util.h"
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



vkb::StaticInit trc::Renderer::_init{
    []() {
        RenderStage::create<DeferredStage>(internal::RenderStages::eDeferred);
        RenderStage::create<ShadowStage>(internal::RenderStages::eShadow);
    },
    []() {}
};

trc::Renderer::Renderer()
    :
    deferredPassId(RenderPass::createAtNextIndex<RenderPassDeferred>(vkb::getSwapchain(), 4).first),
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
    sceneDescriptor.updateActiveScene(scene);
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

auto trc::Renderer::getSceneDescriptor() const noexcept -> const SceneDescriptor&
{
    return sceneDescriptor;
}

auto trc::Renderer::getSceneDescriptorProvider() const noexcept
    -> const DescriptorProviderInterface&
{
    return sceneDescriptor.getProvider();
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



trc::GlobalRenderDataDescriptor::GlobalRenderDataDescriptor()
    :
    provider(*this),
    BUFFER_SECTION_SIZE(util::pad(
        CAMERA_DATA_SIZE + SWAPCHAIN_DATA_SIZE,
        vkb::getPhysicalDevice().properties.limits.minUniformBufferOffsetAlignment
    ))
{
    createResources();
    createDescriptors();
}

auto trc::GlobalRenderDataDescriptor::getProvider() const noexcept
    -> const DescriptorProviderInterface&
{
    return provider;
}

void trc::GlobalRenderDataDescriptor::updateCameraMatrices(const Camera& camera)
{
    auto buf = reinterpret_cast<mat4*>(buffer.map(
        BUFFER_SECTION_SIZE * vkb::getSwapchain().getCurrentFrame(), CAMERA_DATA_SIZE
    ));
    buf[0] = camera.getViewMatrix();
    buf[1] = camera.getProjectionMatrix();
    buf[2] = inverse(camera.getViewMatrix());
    buf[3] = inverse(camera.getProjectionMatrix());
    buffer.unmap();
}

void trc::GlobalRenderDataDescriptor::updateSwapchainData(const vkb::Swapchain& swapchain)
{
    const ui32 currentFrame = vkb::getSwapchain().getCurrentFrame();
    const vec2 res = vec2(static_cast<float>(swapchain.getImageExtent().width),
                          static_cast<float>(swapchain.getImageExtent().height));

    auto buf = reinterpret_cast<vec2*>(buffer.map(
        BUFFER_SECTION_SIZE * currentFrame + CAMERA_DATA_SIZE, SWAPCHAIN_DATA_SIZE
    ));
    buf[0] = swapchain.getMousePosition();
    buf[1] = res;
    buffer.unmap();
}

void trc::GlobalRenderDataDescriptor::createResources()
{
    buffer = vkb::Buffer(
        BUFFER_SECTION_SIZE * vkb::getSwapchain().getFrameCount(),
        vk::BufferUsageFlagBits::eUniformBuffer,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
    );
}

void trc::GlobalRenderDataDescriptor::createDescriptors()
{
    // Create descriptors
    descPool = vkb::getDevice()->createDescriptorPoolUnique(
        vk::DescriptorPoolCreateInfo(
            vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
            vkb::getSwapchain().getFrameCount(),
            std::vector<vk::DescriptorPoolSize>{
                { vk::DescriptorType::eUniformBufferDynamic, 2 }
            }
    ));

    descLayout = vkb::getDevice()->createDescriptorSetLayoutUnique(
        vk::DescriptorSetLayoutCreateInfo({},
            std::vector<vk::DescriptorSetLayoutBinding>{
                {
                    0, vk::DescriptorType::eUniformBufferDynamic, 1,
                    vk::ShaderStageFlagBits::eAllGraphics
                },
                {
                    1, vk::DescriptorType::eUniformBufferDynamic, 1,
                    vk::ShaderStageFlagBits::eAllGraphics
                },
            }
    ));

    descSet = std::move(vkb::getDevice()->allocateDescriptorSetsUnique(
        vk::DescriptorSetAllocateInfo(*descPool, *descLayout)
    )[0]);

    // Update descriptor set
    vk::DescriptorBufferInfo cameraInfo(*buffer, 0, CAMERA_DATA_SIZE);
    vk::DescriptorBufferInfo swapchainInfo(*buffer, CAMERA_DATA_SIZE, SWAPCHAIN_DATA_SIZE);
    std::vector<vk::WriteDescriptorSet> writes{
        vk::WriteDescriptorSet(
            *descSet, 0, 0, 1,
            vk::DescriptorType::eUniformBufferDynamic,
            nullptr, &cameraInfo, nullptr
        ),
        vk::WriteDescriptorSet(
            *descSet, 1, 0, 1,
            vk::DescriptorType::eUniformBufferDynamic,
            nullptr, &swapchainInfo, nullptr
        ),
    };
    vkb::getDevice()->updateDescriptorSets(writes, {});
}

trc::GlobalRenderDataDescriptor::RenderDataDescriptorProvider::RenderDataDescriptorProvider(
    const GlobalRenderDataDescriptor& desc)
    :
    descriptor(desc)
{
}

auto trc::GlobalRenderDataDescriptor::RenderDataDescriptorProvider::getDescriptorSet()
    const noexcept -> vk::DescriptorSet
{
    return *descriptor.descSet;
}

auto trc::GlobalRenderDataDescriptor::RenderDataDescriptorProvider::getDescriptorSetLayout()
    const noexcept -> vk::DescriptorSetLayout
{
    return *descriptor.descLayout;
}

void trc::GlobalRenderDataDescriptor::RenderDataDescriptorProvider::bindDescriptorSet(
    vk::CommandBuffer cmdBuf,
    vk::PipelineBindPoint bindPoint,
    vk::PipelineLayout pipelineLayout,
    ui32 setIndex) const
{
    const ui32 dynamicOffset = descriptor.BUFFER_SECTION_SIZE
                               * vkb::getSwapchain().getCurrentFrame();

    cmdBuf.bindDescriptorSets(
        bindPoint, pipelineLayout, setIndex,
        *descriptor.descSet,
        { dynamicOffset, dynamicOffset }
    );
}
