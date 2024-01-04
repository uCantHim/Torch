#include "trc/ui/torch/GuiIntegration.h"

#include "trc/GuiShaders.h"
#include "trc/PipelineDefinitions.h"
#include "trc/base/Barriers.h"
#include "trc/core/RenderGraph.h"
#include "trc/util/GlmStructuredBindings.h"



auto trc::makeGui(const Device& device, const RenderTarget& renderTarget)
    -> std::pair<u_ptr<ui::Window>, u_ptr<GuiIntegrationPass>>
{
    auto window = std::make_unique<ui::Window>(ui::WindowCreateInfo{
        .windowBackend=std::make_unique<trc::TorchWindowBackend>(renderTarget),
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

    auto renderPass = std::make_unique<GuiIntegrationPass>(device, renderTarget, *window);

    return { std::move(window), std::move(renderPass) };
}

void trc::integrateGui(GuiIntegrationPass& renderPass, RenderGraph& graph)
{
    graph.addPass(trc::guiRenderStage, renderPass);
}



trc::GuiIntegrationPass::GuiIntegrationPass(
    const Device& device,
    RenderTarget renderTarget,
    ui::Window& window)
    :
    RenderPass({}, 1),
    device(device),
    target(renderTarget),
    renderer(device),
    guiImage(device, renderer.getRenderPass(), window.getSize()),
    blendDescLayout([&] {
        std::vector<vk::DescriptorSetLayoutBinding> bindings{
            vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eStorageImage, 1,
                                           vk::ShaderStageFlagBits::eCompute),
            vk::DescriptorSetLayoutBinding(1, vk::DescriptorType::eStorageImage, 1,
                                           vk::ShaderStageFlagBits::eCompute),
        };
        return device->createDescriptorSetLayoutUnique({ {}, bindings });
    }()),
    blendDescSets(renderTarget.getFrameClock()),
    // Compute pipeline
    imageBlendPipelineLayout(makePipelineLayout(device, { *blendDescLayout }, {})),
    imageBlendPipeline(
        makeComputePipeline(
            device, imageBlendPipelineLayout,
            internal::loadShader(trc::ui_impl::pipelines::getImageBlend())
        )
    )
{
    renderThread = std::thread([&, this] {
        while (!stopRenderThread)
        {
            {
                std::lock_guard lock(renderLock);
                renderer.render(window, guiImage);
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    });

    setRenderTarget(renderTarget);
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

    auto image = target.getCurrentImage();
    imageMemoryBarrier(
        cmdBuf,
        image,
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
    auto [x, y] = vec2(target.getSize());
    constexpr float LOCAL_SIZE{ 10.0f };
    cmdBuf.dispatch(glm::ceil(x / LOCAL_SIZE), glm::ceil(y / LOCAL_SIZE), 1);

    imageMemoryBarrier(
        cmdBuf,
        image,
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

void trc::GuiIntegrationPass::setRenderTarget(RenderTarget newTarget)
{
    std::scoped_lock lock(renderLock);

    target = newTarget;
    guiImage = GuiRenderTarget(device, renderer.getRenderPass(), newTarget.getSize());
    createDescriptorSets();
    writeDescriptorSets(guiImage.getFramebuffer().getAttachmentView(0));
}

void trc::GuiIntegrationPass::createDescriptorSets()
{
    const auto& clock = target.getFrameClock();

    blendDescSets = { clock };
    blendDescPool.reset();

    // Descriptor pool
    std::vector<vk::DescriptorPoolSize> poolSizes{
        { vk::DescriptorType::eStorageImage, 1 }, // Render result image
        { vk::DescriptorType::eStorageImage, clock.getFrameCount() }, // Swapchain images
    };
    blendDescPool = device->createDescriptorPoolUnique(
        vk::DescriptorPoolCreateInfo(
            vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
            clock.getFrameCount(),
            poolSizes
        )
    );

    // Descriptor sets
    blendDescSets = {
        clock,
        [this](ui32) -> vk::UniqueDescriptorSet {
            return std::move(device->allocateDescriptorSetsUnique(
                { *blendDescPool, *blendDescLayout }
            )[0]);
        }
    };
}

void trc::GuiIntegrationPass::writeDescriptorSets(vk::ImageView srcImage)
{
    for (ui32 i = 0; i < target.getFrameClock().getFrameCount(); i++)
    {
        vk::DescriptorSet set = *blendDescSets.getAt(i);

        auto view = target.getImageView(i);
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
