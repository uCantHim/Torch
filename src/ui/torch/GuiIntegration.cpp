#include "ui/torch/GuiIntegration.h"

#include <ranges>
#include <vulkan/vulkan.hpp>

#include <vkb/ShaderProgram.h>
#include <vkb/event/Event.h>

#include "Renderer.h"
#include "ui/Element.h"
#include "ui/torch/DrawImplementations.h"



auto trc::initGui(Renderer& renderer) -> u_ptr<ui::Window>
{
    auto window = std::make_unique<ui::Window>(ui::WindowCreateInfo{
        .windowBackend=std::make_unique<trc::TorchWindowBackend>(vkb::getSwapchain()),
        .keyMap = ui::KeyMapping{
            .escape     = static_cast<int>(vkb::Key::escape),
            .backspace  = static_cast<int>(vkb::Key::backspace),
            .enter      = static_cast<int>(vkb::Key::enter),
            .tab        = static_cast<int>(vkb::Key::tab),
            .del        = static_cast<int>(vkb::Key::del),
            .arrowLeft  = static_cast<int>(vkb::Key::arrow_left),
            .arrowRight = static_cast<int>(vkb::Key::arrow_right),
            .arrowUp    = static_cast<int>(vkb::Key::arrow_up),
            .arrowDown  = static_cast<int>(vkb::Key::arrow_down),
        }
    });

    // Notify GUI of mouse clicks
    vkb::on<vkb::MouseClickEvent>([window=window.get()](const vkb::MouseClickEvent& e) {
        if (e.action == vkb::InputAction::press)
        {
            vec2 pos = e.swapchain->getMousePosition();
            window->signalMouseClick(pos.x, pos.y);
        }
    });

    // Notify GUI of key events
    vkb::on<vkb::KeyPressEvent>([window=window.get()](auto& e) {
        window->signalKeyPress(static_cast<int>(e.key));
    });
    vkb::on<vkb::KeyRepeatEvent>([window=window.get()](auto& e) {
        window->signalKeyRepeat(static_cast<int>(e.key));
    });
    vkb::on<vkb::KeyReleaseEvent>([window=window.get()](auto& e) {
        window->signalKeyRelease(static_cast<int>(e.key));
    });
    vkb::on<vkb::CharInputEvent>([window=window.get()](auto& e) {
        window->signalCharInput(e.character);
    });

    // Add gui pass and stage to render graph
    auto renderPass = trc::RenderPass::createAtNextIndex<trc::GuiRenderPass>(
        vkb::getDevice(),
        vkb::getSwapchain(),
        *window
    ).first;
    auto& graph = renderer.getRenderGraph();
    graph.after(trc::RenderStageTypes::getDeferred(), trc::getGuiRenderStage());
    graph.addPass(trc::getGuiRenderStage(), renderPass);

    // Window destruction callback; destroy render pass here because
    // it can't outlive the window or the renderer
    window->onWindowDestruction = [=](ui::Window&) {
        trc::RenderPass::destroy(renderPass);
    };

    return window;
}



auto trc::getGuiRenderStage() -> RenderStageType::ID
{
    constexpr ui32 NUM_SUBPASSES = 1;
    static auto stage = RenderStageType::createAtNextIndex(NUM_SUBPASSES).first;

    return stage;
}



trc::GuiRenderer::GuiRenderer(vkb::Device& device, ui::Window& window)
    :
    device(device),
    window(&window),
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
    outputImage(
        vk::ImageCreateInfo(
            vk::ImageCreateFlags(),
            vk::ImageType::e2D, vk::Format::eR8G8B8A8Unorm,
            { ui32(window.getSize().x), ui32(window.getSize().y), 1 },
            1, 1,
            vk::SampleCountFlagBits::e1,
            vk::ImageTiling::eOptimal,
            vk::ImageUsageFlagBits::eColorAttachment
            | vk::ImageUsageFlagBits::eTransferSrc
            | vk::ImageUsageFlagBits::eStorage
        )
    ),
    outputImageView(outputImage.createView(vk::ImageViewType::e2D, vk::Format::eR8G8B8A8Unorm)),
    framebuffer(
        device->createFramebufferUnique(
            vk::FramebufferCreateInfo(
                vk::FramebufferCreateFlags(),
                *renderPass,
                *outputImageView,
                ui32(window.getSize().x), ui32(window.getSize().y), 1
            )
        )
    ),
    collector(device, *renderPass)
{
    auto [queue, family] = device.getQueueManager().getAnyQueue(vkb::QueueType::graphics);
    cmdPool = device->createCommandPoolUnique(
        vk::CommandPoolCreateInfo(vk::CommandPoolCreateFlagBits::eResetCommandBuffer, family)
    );
    cmdBuf = std::move(device->allocateCommandBuffersUnique(
        vk::CommandBufferAllocateInfo(*cmdPool, vk::CommandBufferLevel::ePrimary, 1)
    )[0]);

    renderQueue = device.getQueueManager().reserveQueue(queue);

    outputImage.changeLayout(
        device,
        vk::ImageLayout::eUndefined, vk::ImageLayout::eGeneral
    );
}

void trc::GuiRenderer::render()
{
    auto drawList = window->draw();
    if (drawList.empty()) return;

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
        vk::ImageLayout::eGeneral, vk::ImageLayout::eColorAttachmentOptimal
    );
    cmdBuf->beginRenderPass(
        vk::RenderPassBeginInfo(*renderPass, *framebuffer, { 0, { size.x, size.y } }, clearValue),
        vk::SubpassContents::eInline
    );

    // Record all element commands
    collector.beginFrame(window->getSize());
    for (const auto& info : drawList)
    {
        collector.drawElement(info);
    }
    collector.endFrame(*cmdBuf);

    cmdBuf->endRenderPass();
    cmdBuf->end();

    device->resetFences(*renderFinishedFence);
    renderQueue.submit(
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

auto trc::GuiRenderer::getOutputImage() const -> vk::Image
{
    return *outputImage;
}

auto trc::GuiRenderer::getOutputImageView() const -> vk::ImageView
{
    return *outputImageView;
}



trc::GuiRenderPass::GuiRenderPass(
    vkb::Device& device,
    const vkb::Swapchain& swapchain,
    ui::Window& window)
    :
    RenderPass({}, 1),
    device(device),
    renderer(device, window),
    blendDescSets(swapchain)
{
    renderThread = std::thread([this] {
        while (!stopRenderThread)
        {
            {
                std::lock_guard lock(renderLock);
                renderer.render();
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    });

    // Descriptor set layout
    std::vector<vk::DescriptorSetLayoutBinding> bindings{
        vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eStorageImage, 1,
                                       vk::ShaderStageFlagBits::eCompute),
        vk::DescriptorSetLayoutBinding(1, vk::DescriptorType::eStorageImage, 1,
                                       vk::ShaderStageFlagBits::eCompute),
    };
    blendDescLayout = device->createDescriptorSetLayoutUnique({ {}, bindings });

    createDescriptorSets(device, swapchain);

    // Compute pipeline
    imageBlendPipeline = [this]() -> Pipeline::ID {
        auto shaderModule = vkb::createShaderModule(
            vkb::readFile(TRC_SHADER_DIR"/ui/image_blend.comp.spv")
        );
        auto layout = makePipelineLayout({ *blendDescLayout }, {});
        auto pipeline = vkb::getDevice()->createComputePipelineUnique(
            {},
            vk::ComputePipelineCreateInfo(
                {},
                vk::PipelineShaderStageCreateInfo(
                    {}, vk::ShaderStageFlagBits::eCompute, *shaderModule, "main"
                ),
                *layout
            )
        ).value;

        return Pipeline::createAtNextIndex(
            std::move(layout),
            std::move(pipeline),
            vk::PipelineBindPoint::eCompute
        ).first;
    }();
}

trc::GuiRenderPass::~GuiRenderPass()
{
    stopRenderThread = true;
    if (renderThread.joinable()) {
        renderThread.join();
    }

    device->waitIdle();
}

void trc::GuiRenderPass::begin(vk::CommandBuffer cmdBuf, vk::SubpassContents)
{
    std::lock_guard lock(renderLock);

    auto swapchainImage = vkb::getSwapchain().getImage(vkb::getSwapchain().getCurrentFrame());
    cmdBuf.pipelineBarrier(
        vk::PipelineStageFlagBits::eAllCommands,
        vk::PipelineStageFlagBits::eAllCommands,
        vk::DependencyFlags(),
        {},
        {},
        vk::ImageMemoryBarrier(
            {}, {}, vk::ImageLayout::ePresentSrcKHR, vk::ImageLayout::eGeneral,
            VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
            swapchainImage,
            vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1)
        )
    );

    auto& computePipeline = Pipeline::at(imageBlendPipeline);
    cmdBuf.bindPipeline(vk::PipelineBindPoint::eCompute, *computePipeline);
    cmdBuf.bindDescriptorSets(
        vk::PipelineBindPoint::eCompute, computePipeline.getLayout(),
        0, **blendDescSets, {}
    );
    const auto [x, y] = vkb::getSwapchain().getImageExtent();
    constexpr float LOCAL_SIZE{ 10.0f };
    cmdBuf.dispatch(glm::ceil(x / LOCAL_SIZE), glm::ceil(y / LOCAL_SIZE), 1);

    cmdBuf.pipelineBarrier(
        vk::PipelineStageFlagBits::eComputeShader,
        vk::PipelineStageFlagBits::eAllCommands,
        vk::DependencyFlags(),
        {},
        {},
        vk::ImageMemoryBarrier(
            {}, {}, vk::ImageLayout::eGeneral, vk::ImageLayout::ePresentSrcKHR,
            VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
            swapchainImage,
            vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1)
        )
    );
}

void trc::GuiRenderPass::end(vk::CommandBuffer)
{
}

void trc::GuiRenderPass::createDescriptorSets(
    const vkb::Device& device,
    const vkb::Swapchain& swapchain)
{
    // Descriptor pool
    std::vector<vk::DescriptorPoolSize> poolSizes{
        { vk::DescriptorType::eStorageImage, 1 }, // Render result image
        { vk::DescriptorType::eStorageImage, swapchain.getFrameCount() }, // Swapchain images
    };
    blendDescPool = device->createDescriptorPoolUnique(
        vk::DescriptorPoolCreateInfo(
            vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
            swapchain.getFrameCount(),
            poolSizes
        )
    );

    // Descriptor sets
    swapchainImageViews.clear();
    blendDescSets = {
        swapchain,
        [this, &device, &swapchain](ui32 imageIndex) -> vk::UniqueDescriptorSet {
            auto set = std::move(device->allocateDescriptorSetsUnique(
                { *blendDescPool, *blendDescLayout }
            )[0]);

            auto view = *swapchainImageViews.emplace_back(swapchain.createImageView(imageIndex));
            vk::DescriptorImageInfo swapchainImageInfo({}, view, vk::ImageLayout::eGeneral);
            vk::DescriptorImageInfo renderResultImageInfo({}, renderer.getOutputImageView(), vk::ImageLayout::eGeneral);
            std::vector<vk::WriteDescriptorSet> writes{
                {
                    *set, 0, 0, 1,
                    vk::DescriptorType::eStorageImage,
                    &renderResultImageInfo, {}, {}
                },
                {
                    *set, 1, 0, 1,
                    vk::DescriptorType::eStorageImage,
                    &swapchainImageInfo, {}, {}
                },
            };
            device->updateDescriptorSets(writes, {});

            return set;
        }
    };
}
