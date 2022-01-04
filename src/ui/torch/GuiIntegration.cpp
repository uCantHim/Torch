#include "ui/torch/GuiIntegration.h"

#include <ranges>

#include <vkb/ShaderProgram.h>
#include <vkb/event/Event.h>

#include "core/RenderLayout.h"
#include "TorchResources.h"



auto trc::initGui(vkb::Device& device, const vkb::Swapchain& swapchain) -> GuiStack
{
    auto window = std::make_unique<ui::Window>(ui::WindowCreateInfo{
        .windowBackend=std::make_unique<trc::TorchWindowBackend>(swapchain),
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
    ui::Window* windowPtr = window.get();

    auto renderer = std::make_unique<GuiRenderer>(device);
    auto renderPass = std::make_unique<GuiIntegrationPass>(device, swapchain, *window, *renderer);

    return {
        .window = std::move(window),
        .renderer = std::move(renderer),
        .renderPass = std::move(renderPass),

        // Notify GUI of mouse clicks
        .mouseClickListener = vkb::on<vkb::MouseClickEvent>(
            [=](const vkb::MouseClickEvent& e) {
                if (e.action == vkb::InputAction::press)
                {
                    vec2 pos = e.swapchain->getMousePosition();
                    windowPtr->signalMouseClick(pos.x, pos.y);
                }
            }
        ),

        // Notify GUI of key events
        .keyPressListener = vkb::on<vkb::KeyPressEvent>([=](auto& e) {
            windowPtr->signalKeyPress(static_cast<int>(e.key));
        }),
        .keyRepeatListener = vkb::on<vkb::KeyRepeatEvent>([=](auto& e) {
            windowPtr->signalKeyRepeat(static_cast<int>(e.key));
        }),
        .keyReleaseListener = vkb::on<vkb::KeyReleaseEvent>([=](auto& e) {
            windowPtr->signalKeyRelease(static_cast<int>(e.key));
        }),
        .charInputListener = vkb::on<vkb::CharInputEvent>([=](auto& e) {
            windowPtr->signalCharInput(e.character);
        })
    };
}



void trc::integrateGui(GuiStack& stack, RenderLayout& layout)
{
    layout.addPass(trc::guiRenderStage, *stack.renderPass);
}



trc::GuiIntegrationPass::GuiIntegrationPass(
    const vkb::Device& device,
    const vkb::Swapchain& swapchain,
    ui::Window& window,
    GuiRenderer& renderer)
    :
    RenderPass({}, 1),
    device(device),
    swapchain(swapchain),
    renderTarget(device, renderer.getRenderPass(), window.getSize()),
    blendDescLayout([&] {
        std::vector<vk::DescriptorSetLayoutBinding> bindings{
            vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eStorageImage, 1,
                                           vk::ShaderStageFlagBits::eCompute),
            vk::DescriptorSetLayoutBinding(1, vk::DescriptorType::eStorageImage, 1,
                                           vk::ShaderStageFlagBits::eCompute),
        };
        return device->createDescriptorSetLayoutUnique({ {}, bindings });
    }()),
    blendDescSets(swapchain),
    // Compute pipeline
    imageBlendPipelineLayout(makePipelineLayout(device, { *blendDescLayout }, {})),
    imageBlendPipeline(
        makeComputePipeline(
            device, imageBlendPipelineLayout,
            vkb::readFile(TRC_SHADER_DIR"/ui/image_blend.comp.spv")
        )
    ),
    swapchainRecreateListener(vkb::on<vkb::SwapchainRecreateEvent>([&](auto& e) {
        if (e.swapchain != &swapchain) return;

        renderTarget = GuiRenderTarget(device, renderer.getRenderPass(), window.getSize());
        createDescriptorSets();
        writeDescriptorSets(renderTarget.getFramebuffer().getAttachmentView(0));
    }))
{
    renderThread = std::thread([&, this] {
        while (!stopRenderThread)
        {
            {
                std::lock_guard lock(renderLock);
                renderer.render(window, renderTarget);
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    });

    createDescriptorSets();
    writeDescriptorSets(renderTarget.getFramebuffer().getAttachmentView(0));
}

trc::GuiIntegrationPass::~GuiIntegrationPass()
{
    stopRenderThread = true;
    if (renderThread.joinable()) {
        renderThread.join();
    }

    device->waitIdle();
}

void trc::GuiIntegrationPass::begin(vk::CommandBuffer cmdBuf, vk::SubpassContents)
{
    std::lock_guard lock(renderLock);

    auto swapchainImage = swapchain.getImage(swapchain.getCurrentFrame());
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

    imageBlendPipeline.bind(cmdBuf);
    cmdBuf.bindDescriptorSets(
        vk::PipelineBindPoint::eCompute, *imageBlendPipeline.getLayout(),
        0, **blendDescSets, {}
    );
    const auto [x, y] = swapchain.getImageExtent();
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

void trc::GuiIntegrationPass::end(vk::CommandBuffer)
{
}

void trc::GuiIntegrationPass::createDescriptorSets()
{
    blendDescSets = { swapchain };
    blendDescPool.reset();

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
        [this](ui32) -> vk::UniqueDescriptorSet {
            return std::move(device->allocateDescriptorSetsUnique(
                { *blendDescPool, *blendDescLayout }
            )[0]);
        }
    };
}

void trc::GuiIntegrationPass::writeDescriptorSets(vk::ImageView srcImage)
{
    for (ui32 i = 0; i < swapchain.getFrameCount(); i++)
    {
        vk::DescriptorSet set = *blendDescSets.getAt(i);

        auto view = *swapchainImageViews.emplace_back(swapchain.createImageView(i));
        vk::DescriptorImageInfo swapchainImageInfo({}, view, vk::ImageLayout::eGeneral);
        vk::DescriptorImageInfo renderResultImageInfo({}, srcImage, vk::ImageLayout::eGeneral);
        std::vector<vk::WriteDescriptorSet> writes{
            {
                set, 0, 0, 1,
                vk::DescriptorType::eStorageImage,
                &renderResultImageInfo, {}, {}
            },
            {
                set, 1, 0, 1,
                vk::DescriptorType::eStorageImage,
                &swapchainImageInfo, {}, {}
            },
        };
        device->updateDescriptorSets(writes, {});
    }
}
