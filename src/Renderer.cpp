#include "Renderer.h"

#include <vkb/util/Timer.h>

#include "PipelineDefinitions.h"
#include "PipelineRegistry.h"



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
        [&, cameraPos = -camera.getViewMatrix()[3]](auto&&, vk::CommandBuffer cmdBuf)
        {
            auto& p = Pipeline::at(internal::Pipelines::eFinalLighting);
            cmdBuf.pushConstants<vec3>(
                p.getLayout(), vk::ShaderStageFlagBits::eFragment, 0, vec3(cameraPos)
            );

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

void trc::Renderer::signalRecreateRequired()
{
    waitForAllFrames();
}

void trc::Renderer::recreate(vkb::Swapchain&)
{
    auto& deferredPass = RenderPassDeferred::emplace<RenderPassDeferred>(0);
    internal::makeAllDrawablePipelines();
    internal::makeFinalLightingPipeline(deferredPass, deferredPass.getInputAttachmentDescriptor());

    createSemaphores();
}

void trc::Renderer::signalRecreateFinished()
{
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
    return *provider;
}

void trc::GlobalRenderDataDescriptor::updateCameraMatrices(const Camera& camera)
{
    auto buf = reinterpret_cast<mat4*>(buffer.map(0, CAMERA_DATA_SIZE));
    buf[0] = camera.getViewMatrix();
    buf[1] = camera.getProjectionMatrix();
    buf[2] = inverse(camera.getViewMatrix());
    buf[3] = inverse(camera.getProjectionMatrix());
    buffer.unmap();
}

void trc::GlobalRenderDataDescriptor::updateSwapchainData(const vkb::Swapchain& swapchain)
{
    const vec2 res = vec2(static_cast<float>(swapchain.getImageExtent().width),
                          static_cast<float>(swapchain.getImageExtent().height));

    auto buf = reinterpret_cast<vec2*>(buffer.map(CAMERA_DATA_SIZE, SWAPCHAIN_DATA_SIZE));
    buf[0] = swapchain.getMousePosition();
    buf[1] = res;
    buffer.unmap();
}

void trc::GlobalRenderDataDescriptor::vulkanStaticInit()
{
    // Create buffer
    buffer = vkb::Buffer(
        CAMERA_DATA_SIZE + SWAPCHAIN_DATA_SIZE,
        vk::BufferUsageFlagBits::eUniformBuffer,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
    );

    // Create descriptors
    descPool = vkb::getDevice()->createDescriptorPoolUnique(
        vk::DescriptorPoolCreateInfo(
            vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
            vkb::getSwapchain().getFrameCount(),
            std::vector<vk::DescriptorPoolSize>{
                { vk::DescriptorType::eUniformBuffer, 2 }
            }
    ));

    descLayout = vkb::getDevice()->createDescriptorSetLayoutUnique(
        vk::DescriptorSetLayoutCreateInfo({},
            std::vector<vk::DescriptorSetLayoutBinding>{
                {
                    0, vk::DescriptorType::eUniformBuffer, 1,
                    vk::ShaderStageFlagBits::eAllGraphics
                },
                {
                    1, vk::DescriptorType::eUniformBuffer, 1,
                    vk::ShaderStageFlagBits::eAllGraphics
                },
            }
    ));

    descSets = std::make_unique<vkb::FrameSpecificObject<vk::UniqueDescriptorSet>>([](ui32)
    {
        return std::move(vkb::getDevice()->allocateDescriptorSetsUnique(
            vk::DescriptorSetAllocateInfo(*descPool, *descLayout)
        )[0]);
    });
    for (auto& set : *descSets)
    {
        vk::DescriptorBufferInfo cameraInfo(*buffer, 0, CAMERA_DATA_SIZE);
        vk::DescriptorBufferInfo swapchainInfo(*buffer, CAMERA_DATA_SIZE, SWAPCHAIN_DATA_SIZE);
        std::vector<vk::WriteDescriptorSet> writes{
            vk::WriteDescriptorSet(
                *set, 0, 0, 1,
                vk::DescriptorType::eUniformBuffer,
                nullptr, &cameraInfo, nullptr
            ),
            vk::WriteDescriptorSet(
                *set, 1, 0, 1,
                vk::DescriptorType::eUniformBuffer,
                nullptr, &swapchainInfo, nullptr
            ),
        };
        vkb::getDevice()->updateDescriptorSets(writes, {});
    }

    // Provider
    provider = std::make_unique<FrameSpecificDescriptorProvider>(
        *descLayout,
        vkb::FrameSpecificObject<vk::DescriptorSet>{
            [](ui32 imageIndex) { return *descSets->getAt(imageIndex); }
        }
    );
}

void trc::GlobalRenderDataDescriptor::vulkanStaticDestroy()
{
    descSets.reset();
    descLayout.reset();
    descPool.reset();

    buffer = {};
}
