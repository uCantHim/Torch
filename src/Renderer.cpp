#include "Renderer.h"

#include "PipelineDefinitions.h"



trc::Renderer::Renderer()
    :
    deferredPass(new RenderPassDeferred()),
    cameraMatrixBuffer(
        sizeof(mat4) * 4,
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

    internal::makeDrawableDeferredPipeline(*deferredPass, cameraDescriptorProvider);
    internal::makeFinalLightingPipeline(*deferredPass,
                                        cameraDescriptorProvider,
                                        deferredPass->getInputAttachmentDescriptor());
}

void trc::Renderer::drawFrame(Scene& scene, const Camera& camera)
{
    auto& device = vkb::VulkanBase::getDevice();
    auto& swapchain = vkb::VulkanBase::getSwapchain();

    // Update
    scene.updateTransforms();
    updateCameraMatrixBuffer(camera);
    bindLightBuffer(scene.getLightBuffer());

    // Add final lighting function to scene
    auto finalLightingFunc = scene.registerDrawFunction(
        1, 1,
        [&](vk::CommandBuffer cmdBuf) {
            cmdBuf.bindVertexBuffers(0, *fullscreenQuadVertexBuffer, vk::DeviceSize(0));
            cmdBuf.draw(6, 1, 0, 0);
        }
    );

    // Acquire image
    device->waitForFences(**frameInFlightFences, true, UINT64_MAX);
    device->resetFences(**frameInFlightFences);
    auto image = swapchain.acquireImage(**imageAcquireSemaphores);

    // Collect commands
    DrawInfo info = {
        .renderPass = deferredPass.get(),
        .camera = &camera
    };
    auto cmdBuf = collector.recordScene(scene, info);

    // Remove fullscreen quad function
    scene.unregisterDrawFunction(finalLightingFunc);

    // Submit command buffers
    auto queue = device->getQueue(device.getPhysicalDevice().queueFamilies.graphicsFamilies[0].index, 0);
    vk::PipelineStageFlags waitStage = vk::PipelineStageFlagBits::eVertexInput;
    queue.submit(
        vk::SubmitInfo(
            **imageAcquireSemaphores,
            waitStage,
            cmdBuf,
            **renderFinishedSemaphores
        ),
        **frameInFlightFences
    );

    // Present frame
    auto presentQueue = device->getQueue(device.getPhysicalDevice().queueFamilies.presentationFamilies[0].index, 0);
    swapchain.presentImage(image, presentQueue, { **renderFinishedSemaphores });
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
            return vkb::VulkanBase::getDevice()->createFenceUnique({ vk::FenceCreateFlagBits::eSignaled });
        }
    );
}

void trc::Renderer::createDescriptors()
{
    std::vector<vk::DescriptorPoolSize> poolSizes = {
        { vk::DescriptorType::eUniformBuffer, 1 }, // Camera buffer
        { vk::DescriptorType::eStorageBuffer, 1 }, // Light buffer
    };
    descPool = vkb::VulkanBase::getDevice()->createDescriptorPoolUnique(
        vk::DescriptorPoolCreateInfo({}, 1, poolSizes)
    );

    // Layout
    std::vector<vk::DescriptorSetLayoutBinding> layoutBindings = {
        vk::DescriptorSetLayoutBinding(
            0,
            vk::DescriptorType::eUniformBuffer, 1,
            vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment
        ),
        vk::DescriptorSetLayoutBinding(
            1,
            vk::DescriptorType::eStorageBuffer, 1,
            vk::ShaderStageFlagBits::eFragment
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
    std::vector<vk::WriteDescriptorSet> writes = {
        vk::WriteDescriptorSet(
            *descSet,
            0, 0, 1, vk::DescriptorType::eUniformBuffer,
            {},
            &cameraBufferInfo
        ),
    };

    vkb::VulkanBase::getDevice()->updateDescriptorSets(writes, {});
}

void trc::Renderer::bindLightBuffer(vk::Buffer lightBuffer)
{
    if (cachedLightBuffer != lightBuffer)
    {
        // Wait until no command buffer uses the descriptor set
        std::vector<vk::Fence> fences;
        frameInFlightFences.foreach([&fences](vk::UniqueFence& fence) {
            fences.push_back(*fence);
        });
        vkb::VulkanBase::getDevice()->waitForFences(fences, true, UINT64_MAX);

        // Update descriptor set
        vk::DescriptorBufferInfo lightBufferInfo(lightBuffer, 0, VK_WHOLE_SIZE);

        std::vector<vk::WriteDescriptorSet> writes = {
            { *descSet, 1, 0, 1, vk::DescriptorType::eStorageBuffer, {}, &lightBufferInfo },
        };
        vkb::VulkanBase::getDevice()->updateDescriptorSets(writes, {});

        cachedLightBuffer = lightBuffer;
    }
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

void trc::Renderer::signalRecreateRequired()
{
    std::vector<vk::Fence> fences;
    frameInFlightFences.foreach([&fences](vk::UniqueFence& fence) {
        fences.push_back(*fence);
    });
    vkb::VulkanBase::getDevice()->waitForFences(
        fences,
        true,
        UINT64_MAX
    );
}

void trc::Renderer::recreate(vkb::Swapchain&)
{
    deferredPass = std::make_unique<RenderPassDeferred>();
    internal::makeDrawableDeferredPipeline(*deferredPass, cameraDescriptorProvider);
    internal::makeFinalLightingPipeline(*deferredPass,
                                        cameraDescriptorProvider,
                                        deferredPass->getInputAttachmentDescriptor());
}

void trc::Renderer::signalRecreateFinished()
{
}
