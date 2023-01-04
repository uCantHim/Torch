#include "trc/ui/torch/GuiIntegration.h"

#include "trc/base/Barriers.h"
#include "trc/base/ShaderProgram.h"
#include "trc/base/event/Event.h"

#include "trc/core/RenderLayout.h"
#include "trc/TorchRenderStages.h"
#include "trc/PipelineDefinitions.h"
#include "trc/GuiShaders.h"



trc::TorchGuiFontLoaderBackend::TorchGuiFontLoaderBackend(GuiRenderer& renderer)
    : renderer(&renderer)
{
}

auto trc::TorchGuiFontLoaderBackend::loadFont(const fs::path& file, ui32 fontSize) -> ui32
{
    const ui32 fontIndex = nextFontIndex++;
    auto& cache = fonts.emplace(fontIndex, new GlyphCache(Face(file, fontSize)));

    renderer->notifyNewFont(fontIndex, *cache);

    return fontIndex;
}

auto trc::TorchGuiFontLoaderBackend::getFontInfo(ui32 fontIndex) -> const Face&
{
    assert(fonts.at(fontIndex) != nullptr);
    return fonts.at(fontIndex)->getFace();
}

auto trc::TorchGuiFontLoaderBackend::getGlyph(ui32 fontIndex, CharCode character)
    -> const GlyphMeta&
{
    assert(fonts.at(fontIndex) != nullptr);
    return fonts.at(fontIndex)->getGlyph(character);
}



auto trc::initGui(Device& device, const Swapchain& swapchain) -> GuiStack
{
    auto renderer = std::make_unique<GuiRenderer>(device);
    auto fontLoader = std::make_unique<TorchGuiFontLoaderBackend>(*renderer);

    auto window = std::make_unique<ui::Window>(ui::WindowCreateInfo{
        .windowBackend=std::make_unique<trc::TorchWindowBackend>(swapchain),
        .fontLoader=*fontLoader,
        .keyMap = ui::KeyMapping{
            .escape     = static_cast<int>(Key::escape),
            .backspace  = static_cast<int>(Key::backspace),
            .enter      = static_cast<int>(Key::enter),
            .tab        = static_cast<int>(Key::tab),
            .del        = static_cast<int>(Key::del),
            .arrowLeft  = static_cast<int>(Key::arrow_left),
            .arrowRight = static_cast<int>(Key::arrow_right),
            .arrowUp    = static_cast<int>(Key::arrow_up),
            .arrowDown  = static_cast<int>(Key::arrow_down),
        }
    });
    ui::Window* windowPtr = window.get();

    auto renderPass = std::make_unique<GuiIntegrationPass>(device, swapchain, *window, *renderer);

    return {
        .window = std::move(window),
        .renderer = std::move(renderer),
        .renderPass = std::move(renderPass),
        .fontLoader = std::move(fontLoader),

        // Notify GUI of mouse clicks
        .mouseClickListener = on<MouseClickEvent>(
            [=](const MouseClickEvent& e) {
                if (e.action == InputAction::press)
                {
                    vec2 pos = e.swapchain->getMousePosition();
                    windowPtr->signalMouseClick(pos.x, pos.y);
                }
            }
        ),

        // Notify GUI of key events
        .keyPressListener = on<KeyPressEvent>([=](auto& e) {
            windowPtr->signalKeyPress(static_cast<int>(e.key));
        }),
        .keyRepeatListener = on<KeyRepeatEvent>([=](auto& e) {
            windowPtr->signalKeyRepeat(static_cast<int>(e.key));
        }),
        .keyReleaseListener = on<KeyReleaseEvent>([=](auto& e) {
            windowPtr->signalKeyRelease(static_cast<int>(e.key));
        }),
        .charInputListener = on<CharInputEvent>([=](auto& e) {
            windowPtr->signalCharInput(e.character);
        })
    };
}



void trc::integrateGui(GuiStack& stack, RenderLayout& layout)
{
    layout.addPass(trc::guiRenderStage, *stack.renderPass);
}



trc::GuiIntegrationPass::GuiIntegrationPass(
    const Device& device,
    const Swapchain& swapchain,
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
            internal::loadShader(trc::ui_impl::pipelines::getImageBlend())
        )
    ),
    swapchainRecreateListener(on<SwapchainRecreateEvent>([&](auto& e) {
        if (e.swapchain != &swapchain) return;

        renderTarget = GuiRenderTarget(device, renderer.getRenderPass(), window.getSize());
        createDescriptorSets();
        writeDescriptorSets(renderTarget.getFramebuffer().getAttachmentView(0));
    }).makeUnique())
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

void trc::GuiIntegrationPass::begin(
    vk::CommandBuffer cmdBuf,
    vk::SubpassContents,
    FrameRenderState&)
{
    std::lock_guard lock(renderLock);

    auto swapchainImage = swapchain.getImage(swapchain.getCurrentFrame());
    imageMemoryBarrier(
        cmdBuf,
        swapchainImage,
        vk::ImageLayout::ePresentSrcKHR,
        vk::ImageLayout::eGeneral,
        vk::PipelineStageFlagBits::eAllGraphics,
        vk::PipelineStageFlagBits::eComputeShader,
        vk::AccessFlagBits::eMemoryWrite,
        vk::AccessFlagBits::eShaderRead | vk::AccessFlagBits::eShaderWrite,
        DEFAULT_SUBRES_RANGE
    );

    imageBlendPipeline.bind(cmdBuf);
    cmdBuf.bindDescriptorSets(
        vk::PipelineBindPoint::eCompute, *imageBlendPipeline.getLayout(),
        0, **blendDescSets, {}
    );
    const auto [x, y] = swapchain.getImageExtent();
    constexpr float LOCAL_SIZE{ 10.0f };
    cmdBuf.dispatch(glm::ceil(x / LOCAL_SIZE), glm::ceil(y / LOCAL_SIZE), 1);

    imageMemoryBarrier(
        cmdBuf,
        swapchainImage,
        vk::ImageLayout::eGeneral,
        vk::ImageLayout::ePresentSrcKHR,
        vk::PipelineStageFlagBits::eComputeShader,
        vk::PipelineStageFlagBits::eHost,
        vk::AccessFlagBits::eShaderWrite,
        vk::AccessFlagBits::eMemoryRead,
        DEFAULT_SUBRES_RANGE
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

        auto view = swapchain.getImageView(i);
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
