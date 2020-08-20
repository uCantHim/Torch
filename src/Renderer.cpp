#include "Renderer.h"

#include <vkb/util/Timer.h>

#include "PipelineDefinitions.h"



trc::Renderer::Renderer()
    :
    cameraMatrixBuffer(
        sizeof(mat4) * 4,
        vk::BufferUsageFlagBits::eUniformBuffer,
        vk::MemoryPropertyFlagBits::eHostCoherent | vk::MemoryPropertyFlagBits::eHostVisible
    ),
    globalDataBuffer(
        sizeof(vec2) * 2,
        vk::BufferUsageFlagBits::eUniformBuffer,
        vk::MemoryPropertyFlagBits::eHostCoherent | vk::MemoryPropertyFlagBits::eHostVisible
    ),
    fullscreenQuadVertexBuffer(
        std::vector<vec3>{
            vec3(-1, 1, 0), vec3(1, 1, 0), vec3(-1, -1, 0),
            vec3(1, 1, 0), vec3(1, -1, 0), vec3(-1, -1, 0)
        },
        vk::BufferUsageFlagBits::eVertexBuffer
    )
{
    createSemaphores();
    createDescriptors();

    initRenderStages();
    addStage(internal::RenderStages::eDeferred, 3);
    addStage(internal::RenderStages::eShadow, 1);

    auto& deferredPass = RenderPass::at(internal::RenderPasses::eDeferredPass);
    internal::makeAllDrawablePipelines(deferredPass, cameraDescriptorProvider);
    internal::makeFinalLightingPipeline(
        deferredPass,
        cameraDescriptorProvider,
        static_cast<RenderPassDeferred&>(deferredPass).getInputAttachmentDescriptor());
}

void trc::Renderer::drawFrame(Scene& scene, const Camera& camera)
{
    auto& device = vkb::VulkanBase::getDevice();
    auto& swapchain = vkb::VulkanBase::getSwapchain();

    // Update
    scene.updateTransforms();
    SceneDescriptor::setActiveScene(scene);
    updateCameraMatrixBuffer(camera);
    updateGlobalDataBuffer(vkb::getSwapchain());

    vec3 cameraPos = camera.getPosition();

    // Add final lighting function to scene
    auto finalLightingFunc = scene.registerDrawFunction(
        internal::RenderStages::eDeferred,
        internal::DeferredSubPasses::eLightingPass,
        internal::Pipelines::eFinalLighting,
        [&, cameraPos](const DrawEnvironment&, vk::CommandBuffer cmdBuf)
        {
            auto& p = GraphicsPipeline::at(internal::Pipelines::eFinalLighting);
            cmdBuf.pushConstants<vec3>(
                p.getLayout(), vk::ShaderStageFlagBits::eFragment, 0, cameraPos
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
    imageAcquireSemaphores = vkb::FrameSpecificObject<vk::UniqueSemaphore>(
        [](ui32) {
            return vkb::VulkanBase::getDevice()->createSemaphoreUnique({});
        }
    );

    renderFinishedSemaphores = vkb::FrameSpecificObject<vk::UniqueSemaphore>(
        [](ui32) {
            return vkb::VulkanBase::getDevice()->createSemaphoreUnique({});
        }
    );

    frameInFlightFences = vkb::FrameSpecificObject<vk::UniqueFence>(
        [](ui32) {
            return vkb::VulkanBase::getDevice()->createFenceUnique(
                { vk::FenceCreateFlagBits::eSignaled }
            );
        }
    );
}

void trc::Renderer::createDescriptors()
{
    std::vector<vk::DescriptorPoolSize> poolSizes = {
        { vk::DescriptorType::eUniformBuffer, 1 }, // Camera buffer
        { vk::DescriptorType::eUniformBuffer, 1 }, // Global data buffer
    };
    descPool = vkb::VulkanBase::getDevice()->createDescriptorPoolUnique(
        vk::DescriptorPoolCreateInfo(
            vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
            1, poolSizes
        )
    );

    // Layout
    std::vector<vk::DescriptorSetLayoutBinding> layoutBindings{
        vk::DescriptorSetLayoutBinding(
            0, vk::DescriptorType::eUniformBuffer, 1,
            vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment
        ),
        vk::DescriptorSetLayoutBinding(
            1, vk::DescriptorType::eUniformBuffer, 1,
            vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment
        ),
    };
    descLayout = vkb::VulkanBase::getDevice()->createDescriptorSetLayoutUnique(
        vk::DescriptorSetLayoutCreateInfo({}, layoutBindings)
    );

    // Set
    descSet = std::move(vkb::VulkanBase::getDevice()->allocateDescriptorSetsUnique(
        vk::DescriptorSetAllocateInfo(*descPool, 1, &*descLayout)
    )[0]);
    cameraDescriptorProvider = { *descLayout, *descSet };

    // Write
    vk::DescriptorBufferInfo cameraBufferInfo(*cameraMatrixBuffer, 0, VK_WHOLE_SIZE);
    vk::DescriptorBufferInfo globalDataBufferInfo(*globalDataBuffer, 0, VK_WHOLE_SIZE);
    std::vector<vk::WriteDescriptorSet> writes = {
        vk::WriteDescriptorSet(
            *descSet, 0, 0, 1, vk::DescriptorType::eUniformBuffer,
            {}, &cameraBufferInfo
        ),
        vk::WriteDescriptorSet(
            *descSet, 1, 0, 1, vk::DescriptorType::eUniformBuffer,
            {}, &globalDataBufferInfo
        ),
    };

    vkb::VulkanBase::getDevice()->updateDescriptorSets(writes, {});
}

void trc::Renderer::updateCameraMatrixBuffer(const Camera& camera)
{
    auto buf = reinterpret_cast<mat4*>(cameraMatrixBuffer.map());
    buf[0] = camera.getViewMatrix();
    buf[1] = camera.getProjectionMatrix();
    buf[2] = inverse(camera.getViewMatrix());
    buf[3] = inverse(camera.getProjectionMatrix());
    cameraMatrixBuffer.unmap();
}

void trc::Renderer::updateGlobalDataBuffer(const vkb::Swapchain& swapchain)
{
    const vec2 res = vec2(static_cast<float>(swapchain.getImageExtent().width),
                    static_cast<float>(swapchain.getImageExtent().height));

    auto buf = reinterpret_cast<vec2*>(globalDataBuffer.map());
    buf[0] = swapchain.getMousePosition();
    buf[1] = res;
    globalDataBuffer.unmap();
}

void trc::Renderer::signalRecreateRequired()
{
    waitForAllFrames();
}

void trc::Renderer::recreate(vkb::Swapchain&)
{
    auto& deferredPass = RenderPassDeferred::emplace<RenderPassDeferred>(0);
    internal::makeAllDrawablePipelines(deferredPass, cameraDescriptorProvider);
    internal::makeFinalLightingPipeline(deferredPass,
                                        cameraDescriptorProvider,
                                        deferredPass.getInputAttachmentDescriptor());

    createSemaphores();
}

void trc::Renderer::signalRecreateFinished()
{
}

void trc::Renderer::waitForAllFrames(ui64 timeoutNs)
{
    std::vector<vk::Fence> fences;
    frameInFlightFences.foreach([&fences](vk::UniqueFence& fence) {
        fences.push_back(*fence);
    });
    auto result = vkb::VulkanBase::getDevice()->waitForFences(fences, true, timeoutNs);
    if (result == vk::Result::eTimeout) {
        std::cout << "Timeout in Renderer::waitForAllFrames!\n";
    }
}
