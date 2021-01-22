#include "ui/torch/GuiIntegration.h"

#include <ranges>
#include <vulkan/vulkan.hpp>

#include "PipelineBuilder.h"
#include "ui/Element.h"



namespace trc
{
    auto makeQuadPipeline(vk::RenderPass renderPass, ui32 subPass) -> Pipeline::ID
    {
        vkb::ShaderProgram program(
            TRC_SHADER_DIR"/ui/quad.vert.spv",
            TRC_SHADER_DIR"/ui/quad.frag.spv"
        );

        auto layout = trc::makePipelineLayout(
            {},
            {
                vk::PushConstantRange(
                    vk::ShaderStageFlagBits::eVertex,
                    0,
                    // pos, size
                    sizeof(vec2) * 2
                    // color
                    + sizeof(vec4)
                )
            }
        );

        auto pipeline = GraphicsPipelineBuilder::create()
            .setProgram(program)
            .addVertexInputBinding(
                vk::VertexInputBindingDescription(0, sizeof(vec2), vk::VertexInputRate::eVertex),
                {
                    vk::VertexInputAttributeDescription(0, 0, vk::Format::eR32G32Sfloat, 0)
                }
            )
            .addViewport({})
            .addScissorRect({})
            .addDynamicState(vk::DynamicState::eViewport)
            .addDynamicState(vk::DynamicState::eScissor)
            .setCullMode(vk::CullModeFlagBits::eNone)
            .addColorBlendAttachment(DEFAULT_COLOR_BLEND_ATTACHMENT_DISABLED)
            .setColorBlending({}, false, {}, {})
            .build(*vkb::getDevice(), *layout, renderPass, subPass);

        return Pipeline::createAtNextIndex(
            std::move(layout),
            std::move(pipeline),
            vk::PipelineBindPoint::eGraphics
        ).first;
    }
}



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
    ),
    quadPipeline(makeQuadPipeline(*renderPass, 0))
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
        [](const ui::DrawInfo& a, const ui::DrawInfo& b) {
            return a.typeInfo.index() < b.typeInfo.index();
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

    for (const ui::DrawInfo& info : drawList)
    {
        std::visit([this, &info, cmdBuf=*cmdBuf](auto type) {
            if constexpr (std::is_same_v<ui::NoType, decltype(type)>)
            {
                // Draw generic quad
                drawQuad(info, cmdBuf);
            }
            else if constexpr (std::is_same_v<ui::TextDrawInfo, decltype(type)>)
            {
                // Draw text
                std::cout << "Warning: Encountered text element, which is"
                    << " not yet implemented by the renderer\n";
            }
        }, info.typeInfo);
    }

    cmdBuf->endRenderPass();
    cmdBuf->end();

    auto fence = vkb::getDevice()->createFenceUnique(vk::FenceCreateInfo());
    renderQueue.submit(vk::SubmitInfo(0, nullptr, nullptr, 1, &*cmdBuf, 0, nullptr), *fence);
    auto result = vkb::getDevice()->waitForFences(*fence, true, UINT64_MAX);
    std::cout << "Fence wait result: " << vk::to_string(result) << "\n";
}

auto trc::GuiRenderer::getOutputImage() -> vk::Image
{
    return *outputImage;
}

void trc::GuiRenderer::drawQuad(const ui::DrawInfo& info, vk::CommandBuffer cmdBuf)
{
    auto& p = Pipeline::at(quadPipeline);
    cmdBuf.bindPipeline(vk::PipelineBindPoint::eGraphics, *p);
    auto size = vkb::getSwapchain().getImageExtent();
    cmdBuf.setViewport(0, vk::Viewport(0, 0, size.width, size.height, 0.0f, 1.0f));
    cmdBuf.setScissor(0, vk::Rect2D({ 0, 0 }, { size.width, size.height }));

    cmdBuf.pushConstants<vec2>(
        p.getLayout(), vk::ShaderStageFlagBits::eVertex,
        0, { info.pos, info.size }
    );
    cmdBuf.pushConstants<vec4>(
        p.getLayout(), vk::ShaderStageFlagBits::eVertex,
        static_cast<ui32>(sizeof(vec2) * 2), std::get<vec4>(info.color)
    );

    cmdBuf.bindVertexBuffers(0, **quadVertexBuffer, { 0 });
    cmdBuf.draw(6, 1, 0, 0);
}



vkb::StaticInit trc::GuiRenderer::_init{
    [] {
        quadVertexBuffer = std::make_unique<vkb::DeviceLocalBuffer>(
            std::vector<vec2>{
                vec2(0, 0), vec2(1, 1), vec2(0, 1),
                vec2(0, 0), vec2(1, 0), vec2(1, 1)
            },
            vk::BufferUsageFlagBits::eVertexBuffer
        );
    },
    [] {
        quadVertexBuffer.reset();
    }
};

trc::GuiRenderPass::GuiRenderPass(
    ui::Window& window,
    vkb::FrameSpecificObject<vk::Image> renderTargets)
    :
    RenderPass({}, 1),
    renderer(window),
    renderTargets(std::move(renderTargets))
{
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
