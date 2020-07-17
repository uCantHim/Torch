#include "Renderer.h"



trc::Renderer::Renderer()
{
    internal::initRenderEnvironment();
    createSemaphores();
    createFramebuffer();
}

void trc::Renderer::drawFrame(Scene& scene)
{
    auto& device = vkb::VulkanBase::getDevice();
    auto& swapchain = vkb::VulkanBase::getSwapchain();
    auto swapchainSize = swapchain.getImageExtent();

    device->waitForFences(**frameInFlightFences, true, UINT64_MAX);
    device->resetFences(**frameInFlightFences);
    auto image = swapchain.acquireImage(**imageAcquireSemaphores);

    DrawInfo info = {
        .renderPass = &RenderPass::at(0),
        .framebuffer = **framebuffers,
        .viewport = { {0, 0}, {swapchainSize.width, swapchainSize.height} }
    };
    auto cmdBuf = collector.recordScene(scene, info);

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
    internal::initRenderEnvironment();
    createFramebuffer();
}

void trc::Renderer::signalRecreateFinished()
{
}
