#include "ui/torch/GuiIntegration.h"

#include <ranges>
#include <vulkan/vulkan.hpp>

#include "ui/Element.h"
#include "ui/torch/DrawImplementations.h"



auto trc::getGuiRenderStage() -> RenderStageType::ID
{
    constexpr ui32 NUM_SUBPASSES = 1;
    static auto stage = RenderStageType::createAtNextIndex(NUM_SUBPASSES).first;

    return stage;
}



trc::GuiRenderer::GuiRenderer(ui::Window& window)
    :
    window(&window),
    renderPass(
        []() {
            std::vector<vk::AttachmentDescription> attachments{
                vk::AttachmentDescription(
                    vk::AttachmentDescriptionFlags(),
                    vk::Format::eR8G8B8A8Unorm,
                    vk::SampleCountFlagBits::e1,
                    vk::AttachmentLoadOp::eClear,
                    vk::AttachmentStoreOp::eStore,
                    vk::AttachmentLoadOp::eDontCare, // stencil
                    vk::AttachmentStoreOp::eDontCare, // stencil
                    vk::ImageLayout::eColorAttachmentOptimal,
                    vk::ImageLayout::eTransferSrcOptimal
                )
            };
            vk::AttachmentReference colorAttachment(0, vk::ImageLayout::eColorAttachmentOptimal);
            std::vector<vk::SubpassDescription> subpasses{
                vk::SubpassDescription(
                    vk::SubpassDescriptionFlags(),
                    vk::PipelineBindPoint::eGraphics,
                    0, nullptr, // input
                    1, &colorAttachment,
                    nullptr,    // resolve
                    nullptr,    // depth
                    0, nullptr  // some other attachment
                )
            };
            std::vector<vk::SubpassDependency> dependencies{
                vk::SubpassDependency(
                    VK_SUBPASS_EXTERNAL, 0,
                    vk::PipelineStageFlagBits::eAllGraphics,
                    vk::PipelineStageFlagBits::eTopOfPipe,
                    vk::AccessFlags(), vk::AccessFlags()
                )
            };
            return vkb::getDevice()->createRenderPassUnique(
                vk::RenderPassCreateInfo(
                    vk::RenderPassCreateFlags(),
                    attachments,
                    subpasses,
                    dependencies
                )
            );
        }()
    ),
    outputImage(
        vk::ImageCreateInfo(
            vk::ImageCreateFlags(),
            vk::ImageType::e2D, vk::Format::eR8G8B8A8Unorm,
            { ui32(window.getSize().x), ui32(window.getSize().y), 1 },
            1, 1,
            vk::SampleCountFlagBits::e1,
            vk::ImageTiling::eOptimal,
            vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferSrc
        )
    ),
    outputImageView(outputImage.createView(vk::ImageViewType::e2D, vk::Format::eR8G8B8A8Unorm)),
    framebuffer(
        vkb::getDevice()->createFramebufferUnique(
            vk::FramebufferCreateInfo(
                vk::FramebufferCreateFlags(),
                *renderPass,
                *outputImageView,
                ui32(window.getSize().x), ui32(window.getSize().y), 1
            )
        )
    )
{
    auto [queue, family] = vkb::getDevice().getQueueManager().getAnyQueue(vkb::QueueType::graphics);
    cmdPool = vkb::getDevice()->createCommandPoolUnique(
        vk::CommandPoolCreateInfo(vk::CommandPoolCreateFlagBits::eResetCommandBuffer, family)
    );
    cmdBuf = std::move(vkb::getDevice()->allocateCommandBuffersUnique(
        vk::CommandBufferAllocateInfo(*cmdPool, vk::CommandBufferLevel::ePrimary, 1)
    )[0]);

    renderQueue = vkb::getDevice().getQueueManager().reserveQueue(queue);

    outputImage.changeLayout(
        vkb::getDevice(),
        vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferSrcOptimal
    );
}

void trc::GuiRenderer::render()
{
    auto drawList = window->draw();

    // Sort draw list by type of drawable
    std::ranges::sort(
        drawList,
        [](const auto& a, const auto& b) {
            return a.type.index() < b.type.index();
        }
    );

    cmdBuf->begin(vk::CommandBufferBeginInfo());

    const uvec2 size = window->getSize();
    vk::ClearValue clearValue{
        vk::ClearColorValue(std::array<float, 4>{{ 0.0f, 0.0f, 0.0f, 0.0f }})
    };
    outputImage.changeLayout(
        *cmdBuf,
        vk::ImageLayout::eTransferSrcOptimal, vk::ImageLayout::eColorAttachmentOptimal
    );
    cmdBuf->beginRenderPass(
        vk::RenderPassBeginInfo(*renderPass, *framebuffer, { 0, { size.x, size.y } }, clearValue),
        vk::SubpassContents::eInline
    );

    // Record all element commands
    for (const auto& info : drawList)
    {
        internal::drawElement(info, *cmdBuf);
    }

    cmdBuf->endRenderPass();
    cmdBuf->end();

    auto fence = vkb::getDevice()->createFenceUnique(vk::FenceCreateInfo());
    renderQueue.submit(vk::SubmitInfo(0, nullptr, nullptr, 1, &*cmdBuf, 0, nullptr), *fence);
    auto result = vkb::getDevice()->waitForFences(*fence, true, UINT64_MAX);
    std::cout << "Fence wait result: " << vk::to_string(result) << "\n";
}

auto trc::GuiRenderer::getRenderPass() const -> vk::RenderPass
{
    return *renderPass;
}

auto trc::GuiRenderer::getOutputImage() const -> vk::Image
{
    return *outputImage;
}



trc::GuiRenderPass::GuiRenderPass(
    ui::Window& window,
    vkb::FrameSpecificObject<vk::Image> renderTargets)
    :
    RenderPass({}, 1),
    renderer(window),
    renderTargets(std::move(renderTargets))
{
    internal::initGuiDraw(renderer.getRenderPass());

    std::thread([this] {
        while (!stopRenderThread)
        {
            {
                std::lock_guard lock(renderLock);
                renderer.render();
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    }).detach();
}

trc::GuiRenderPass::~GuiRenderPass()
{
    stopRenderThread = true;
    if (renderThread.joinable()) {
        renderThread.join();
    }
}

void trc::GuiRenderPass::begin(vk::CommandBuffer cmdBuf, vk::SubpassContents)
{
    std::lock_guard lock(renderLock);

    //cmdBuf.pipelineBarrier(
    //    vk::PipelineStageFlagBits::eAllCommands,
    //    vk::PipelineStageFlagBits::eAllCommands,
    //    vk::DependencyFlags(),
    //    {},
    //    {},
    //    vk::ImageMemoryBarrier(
    //        {}, {}, vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::eTransferSrcOptimal,
    //        VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
    //        renderer.getOutputImage(),
    //        vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1)
    //    )
    //);

    auto swapchainImage = vkb::getSwapchain().getImage(vkb::getSwapchain().getCurrentFrame());
    cmdBuf.pipelineBarrier(
        vk::PipelineStageFlagBits::eAllCommands,
        vk::PipelineStageFlagBits::eAllCommands,
        vk::DependencyFlags(),
        {},
        {},
        vk::ImageMemoryBarrier(
            {}, {}, vk::ImageLayout::ePresentSrcKHR, vk::ImageLayout::eTransferDstOptimal,
            VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
            swapchainImage,
            vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1)
        )
    );

    cmdBuf.copyImage(
        renderer.getOutputImage(),
        vk::ImageLayout::eTransferSrcOptimal,
        swapchainImage,
        vk::ImageLayout::eTransferDstOptimal,
        vk::ImageCopy(
            vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, 0, 0, 1),
            { 0, 0, 0 }, // src offset
            vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, 0, 0, 1),
            { 0, 0, 0 }, // dst offset
            vk::Extent3D{ vkb::getSwapchain().getImageExtent(),  1 }
        )
    );

    cmdBuf.pipelineBarrier(
        vk::PipelineStageFlagBits::eAllCommands,
        vk::PipelineStageFlagBits::eAllCommands,
        vk::DependencyFlags(),
        {},
        {},
        vk::ImageMemoryBarrier(
            {}, {}, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::ePresentSrcKHR,
            VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
            swapchainImage,
            vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1)
        )
    );
}

void trc::GuiRenderPass::end(vk::CommandBuffer)
{
}
