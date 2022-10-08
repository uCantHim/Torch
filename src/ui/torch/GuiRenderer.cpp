#include "trc/ui/torch/GuiRenderer.h"

#include "trc/ui/torch/DrawImplementations.h"



trc::GuiRenderTarget::GuiRenderTarget(
    const Device& device,
    vk::RenderPass renderPass,
    uvec2 size)
    :
    image(
        device,
        vk::ImageCreateInfo(
            vk::ImageCreateFlags(),
            vk::ImageType::e2D, vk::Format::eR8G8B8A8Unorm,
            { size.x, size.y, 1 },
            1, 1,
            vk::SampleCountFlagBits::e1,
            vk::ImageTiling::eOptimal,
            vk::ImageUsageFlagBits::eColorAttachment
            | vk::ImageUsageFlagBits::eTransferSrc
            | vk::ImageUsageFlagBits::eStorage
        )
    ),
    framebuffer([&] {
        std::vector<vk::UniqueImageView> imageViews;
        imageViews.push_back(image.createView(vk::ImageViewType::e2D, vk::Format::eR8G8B8A8Unorm));
        return Framebuffer(device, renderPass, size, std::move(imageViews));
    }())
{
    // Set initial image layout
    device.executeCommands(QueueType::graphics, [&](auto cmdBuf)
    {
        image.barrier(cmdBuf, vk::ImageLayout::eUndefined, vk::ImageLayout::eGeneral);
    });
}

auto trc::GuiRenderTarget::getSize() const -> uvec2
{
    return image.getSize();
}

auto trc::GuiRenderTarget::getImage() -> Image&
{
    return image;
}

auto trc::GuiRenderTarget::getFramebuffer() const -> const Framebuffer&
{
    return framebuffer;
}



trc::GuiRenderer::GuiRenderer(Device& device)
    :
    device(device),
    renderFinishedFence(device->createFenceUnique({})),
    renderPass(
        [&device]() {
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
                    vk::ImageLayout::eGeneral
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
            return device->createRenderPassUnique(
                vk::RenderPassCreateInfo(
                    vk::RenderPassCreateFlags(),
                    attachments,
                    subpasses,
                    dependencies
                )
            );
        }()
    ),
    clearValue(vk::ClearColorValue(std::array<float, 4>{{ 0.0f, 0.0f, 0.0f, 0.0f }})),
    collector(device, *this)
{
    auto [queue, family] = device.getQueueManager().getAnyQueue(QueueType::graphics);
    cmdPool = device->createCommandPoolUnique(
        vk::CommandPoolCreateInfo(vk::CommandPoolCreateFlagBits::eResetCommandBuffer, family)
    );
    cmdBuf = std::move(device->allocateCommandBuffersUnique(
        vk::CommandBufferAllocateInfo(*cmdPool, vk::CommandBufferLevel::ePrimary, 1)
    )[0]);

    renderQueue = device.getQueueManager().reserveQueue(queue);
}

void trc::GuiRenderer::render(ui::Window& window, GuiRenderTarget& target)
{
    auto drawList = window.draw();
    if (drawList.empty()) return;

    // Sort draw list by type of drawable
    std::ranges::sort(
        drawList,
        [](const auto& a, const auto& b) {
            return a.type.index() < b.type.index();
        }
    );

    cmdBuf->begin(vk::CommandBufferBeginInfo());
    target.getImage().barrier(
        *cmdBuf,
        vk::ImageLayout::eGeneral, vk::ImageLayout::eColorAttachmentOptimal,
        vk::PipelineStageFlagBits::eAllCommands,
        vk::PipelineStageFlagBits::eColorAttachmentOutput,
        vk::AccessFlagBits::eShaderRead,
        vk::AccessFlagBits::eColorAttachmentWrite
    );

    const uvec2 size = target.getSize();
    cmdBuf->beginRenderPass(
        vk::RenderPassBeginInfo(
            *renderPass,
            *target.getFramebuffer(),
            { 0, { size.x, size.y } },
            clearValue
        ),
        vk::SubpassContents::eInline
    );

    // Record all element commands
    collector.beginFrame();
    for (const auto& info : drawList)
    {
        collector.drawElement(info);
    }
    collector.endFrame(*cmdBuf, size);

    cmdBuf->endRenderPass();
    cmdBuf->end();

    device->resetFences(*renderFinishedFence);
    renderQueue.waitSubmit(
        vk::SubmitInfo(0, nullptr, nullptr, 1, &*cmdBuf, 0, nullptr),
        *renderFinishedFence
    );
    auto result = device->waitForFences(*renderFinishedFence, true, UINT64_MAX);
    if (result != vk::Result::eSuccess) {
        throw std::runtime_error("vkWaitForFences returned error while waiting for GUI render fence");
    }
}

auto trc::GuiRenderer::getRenderPass() const -> vk::RenderPass
{
    return *renderPass;
}
