#include "Renderer.h"

#include <vkb/util/Timer.h>
#include <vkb/event/EventHandler.h>
#include <vkb/event/WindowEvents.h>

#include "utils/Util.h"
#include "PipelineDefinitions.h"
#include "PipelineRegistry.h"



vkb::StaticInit trc::Renderer::_init{
    []() {
        RenderStage::create<DeferredStage>(internal::RenderStages::eDeferred);
        RenderStage::create<ShadowStage>(internal::RenderStages::eShadow);
    },
    []() {}
};

trc::Renderer::Renderer()
    :
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
    vkb::EventHandler<vkb::PreSwapchainRecreateEvent>::addListener([this](const auto&) {
        waitForAllFrames();
    });
    // Post recreate, create the required resources
    vkb::EventHandler<vkb::SwapchainRecreateEvent>::addListener([this](const auto&) {
        RenderPassDeferred::emplace<RenderPassDeferred>(0);
        PipelineRegistry::recreateAll();
        createSemaphores();
    });

    addStage(internal::RenderStages::eDeferred, 3);
    addStage(internal::RenderStages::eShadow, 1);

    PipelineRegistry::registerPipeline(internal::makeAllDrawablePipelines);
    PipelineRegistry::registerPipeline([&]() {
        auto& deferredPass = static_cast<RenderPassDeferred&>(
            RenderPass::at(internal::RenderPasses::eDeferredPass)
        );
        internal::makeFinalLightingPipeline(
            deferredPass,
            deferredPass.getInputAttachmentDescriptor()
        );
    });
    PipelineRegistry::recreateAll();
}

void trc::Renderer::drawFrame(Scene& scene, const Camera& camera)
{
    auto& device = vkb::VulkanBase::getDevice();
    auto& swapchain = vkb::VulkanBase::getSwapchain();

    // Update
    scene.updateTransforms();
    SceneDescriptor::setActiveScene(scene);
    GlobalRenderDataDescriptor::updateCameraMatrices(camera);
    GlobalRenderDataDescriptor::updateSwapchainData(vkb::getSwapchain());

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
    auto queue = vkb::getQueueProvider().getQueue(vkb::QueueType::graphics);
    vk::PipelineStageFlags waitStage = vk::PipelineStageFlagBits::eVertexInput;
    queue.submit(
        vk::SubmitInfo(
            **imageAcquireSemaphores,
            waitStage,
            cmdBufs,
            **renderFinishedSemaphores
        ),
        **frameInFlightFences
    );

    // Present frame
    auto presentQueue = vkb::getQueueProvider().getQueue(vkb::QueueType::presentation);
    swapchain.presentImage(image, presentQueue, { **renderFinishedSemaphores });
}

void trc::Renderer::addStage(RenderStage::ID stage, ui32 priority)
{
    auto it = renderStages.begin();
    while (it != renderStages.end() && it->second < priority) it++;

    renderStages.insert(it, { stage, priority });
    commandCollectors.emplace_back();
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



auto trc::GlobalRenderDataDescriptor::getProvider() noexcept -> const DescriptorProviderInterface&
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

void trc::GlobalRenderDataDescriptor::vulkanStaticInit()
{
    BUFFER_SECTION_SIZE = util::pad(
        CAMERA_DATA_SIZE + SWAPCHAIN_DATA_SIZE,
        vkb::getPhysicalDevice().properties.limits.minUniformBufferOffsetAlignment
    );

    // Create buffer
    buffer = vkb::Buffer(
        BUFFER_SECTION_SIZE * vkb::getSwapchain().getFrameCount(),
        vk::BufferUsageFlagBits::eUniformBuffer,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
    );

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

void trc::GlobalRenderDataDescriptor::vulkanStaticDestroy()
{
    descSet.reset();
    descLayout.reset();
    descPool.reset();

    buffer = {};
}

auto trc::GlobalRenderDataDescriptor::RenderDataDescriptorProvider::getDescriptorSet()
    const noexcept -> vk::DescriptorSet
{
    return *GlobalRenderDataDescriptor::descSet;
}

auto trc::GlobalRenderDataDescriptor::RenderDataDescriptorProvider::getDescriptorSetLayout()
    const noexcept -> vk::DescriptorSetLayout
{
    return *GlobalRenderDataDescriptor::descLayout;
}

void trc::GlobalRenderDataDescriptor::RenderDataDescriptorProvider::bindDescriptorSet(
    vk::CommandBuffer cmdBuf,
    vk::PipelineBindPoint bindPoint,
    vk::PipelineLayout pipelineLayout,
    ui32 setIndex) const
{
    const ui32 dynamicOffset = BUFFER_SECTION_SIZE * vkb::getSwapchain().getCurrentFrame();

    cmdBuf.bindDescriptorSets(
        bindPoint, pipelineLayout, setIndex,
        *GlobalRenderDataDescriptor::descSet,
        { dynamicOffset, dynamicOffset }
    );
}
