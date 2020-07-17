#include "Renderer.h"



trc::Renderer::Renderer()
    :
    cameraMatrixBuffer(
        sizeof(mat4) * 4,
        vk::BufferUsageFlagBits::eUniformBuffer,
        vk::MemoryPropertyFlagBits::eHostCoherent | vk::MemoryPropertyFlagBits::eHostVisible
    )
{
    internal::makeRenderPasses();
    createSemaphores();
    createFramebuffer();
    createDescriptors();

    internal::makePipelines({ *descLayout, *descSet });
}

void trc::Renderer::drawFrame(Scene& scene, const Camera& camera)
{
    auto& device = vkb::VulkanBase::getDevice();
    auto& swapchain = vkb::VulkanBase::getSwapchain();

    device->waitForFences(**frameInFlightFences, true, UINT64_MAX);
    device->resetFences(**frameInFlightFences);
    auto image = swapchain.acquireImage(**imageAcquireSemaphores);

    // Collect commands
    updateCameraMatrixBuffer(camera);
    DrawInfo info = {
        .renderPass = &RenderPass::at(0),
        .framebuffer = **framebuffers,
        .camera = &camera
    };
    auto cmdBuf = collector.recordScene(scene, info);

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

void trc::Renderer::createFramebuffer()
{
    framebuffers = {};
    framebufferImageViews.clear();
    framebufferImageViews.reserve(2);
    framebufferAttachmentImages.clear();

    auto& swapchain = vkb::VulkanBase::getSwapchain();

    // Color image views
    auto& colorImageViews = framebufferImageViews.emplace_back(swapchain.createImageViews());

    // Depth images
    auto& depthImages = framebufferAttachmentImages.emplace_back(
        [&swapchain](uint32_t) {
            return vkb::Image(vk::ImageCreateInfo(
                {},
                vk::ImageType::e2D,
                vk::Format::eD24UnormS8Uint,
                vk::Extent3D{ swapchain.getImageExtent(), 1 },
                1, 1, vk::SampleCountFlagBits::e1,
                vk::ImageTiling::eOptimal,
                vk::ImageUsageFlagBits::eDepthStencilAttachment
            ));
        }
    );

    // Depth image views
    auto& depthImageViews = framebufferImageViews.emplace_back(
        [&depthImages](uint32_t imageIndex) {
            return depthImages.getAt(imageIndex).createView(
                vk::ImageViewType::e2D, vk::Format::eD24UnormS8Uint, vk::ComponentMapping(),
                vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eDepth, 0, 1, 0, 1)
            );
        }
    );

    framebuffers = vkb::FrameSpecificObject<vk::UniqueFramebuffer>(
        [&](uint32_t imageIndex) -> vk::UniqueFramebuffer
        {
            std::vector<vk::ImageView> imageViews = {
                *colorImageViews.getAt(imageIndex),
                *depthImageViews.getAt(imageIndex),
            };

            return vkb::VulkanBase::getDevice()->createFramebufferUnique(
                vk::FramebufferCreateInfo(
                    {},
                    *trc::RenderPass::at(0),
                    static_cast<uint32_t>(imageViews.size()), imageViews.data(),
                    swapchain.getImageExtent().width, swapchain.getImageExtent().height,
                    1 // layers
                )
            );
        }
    );
}

void trc::Renderer::createDescriptors()
{
    std::vector<vk::DescriptorPoolSize> poolSizes = {
        { vk::DescriptorType::eUniformBuffer, 1 }, // Camera buffer
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
    };
    descLayout = vkb::VulkanBase::getDevice()->createDescriptorSetLayoutUnique(
        vk::DescriptorSetLayoutCreateInfo({}, layoutBindings)
    );

    // Set
    descSet = std::move(vkb::VulkanBase::getDevice()->allocateDescriptorSetsUnique(
        vk::DescriptorSetAllocateInfo(*descPool, 1, &*descLayout)
    )[0]);

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
    internal::makeRenderPasses();
    internal::makePipelines({ *descLayout, *descSet });
    createFramebuffer();
}

void trc::Renderer::signalRecreateFinished()
{
}
