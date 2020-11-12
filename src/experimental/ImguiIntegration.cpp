#include "experimental/ImguiIntegration.h"

#include <vkb/event/Event.h>
#include <trc/PipelineBuilder.h>



trc::experimental::ImGuiRenderStage::ImGuiRenderStage()
    : RenderStage(RenderStageType::at(IMGUI_RENDER_STAGE_TYPE_ID))
{
    // Create renderpass
    auto& renderPass = RenderPass::replace<ImGuiRenderPass>(IMGUI_RENDERPASS_INDEX);
    addRenderPass(IMGUI_RENDERPASS_INDEX);

    // Standard ImGui initialization stuff
    IMGUI_CHECKVERSION();
    ig::CreateContext();
    ImGuiIO& io = ig::GetIO(); (void) io;

    ig::StyleColorsDark();

    // Create disgusting descriptor pool for internal ImGui usage
    const std::vector<vk::DescriptorPoolSize> poolSizes{
        { vk::DescriptorType::eSampler, 1000 },
        { vk::DescriptorType::eCombinedImageSampler, 1000 },
        { vk::DescriptorType::eSampledImage, 1000 },
        { vk::DescriptorType::eStorageImage, 1000 },
        { vk::DescriptorType::eUniformTexelBuffer, 1000 },
        { vk::DescriptorType::eStorageTexelBuffer, 1000 },
        { vk::DescriptorType::eUniformBuffer, 1000 },
        { vk::DescriptorType::eStorageBuffer, 1000 },
        { vk::DescriptorType::eUniformBufferDynamic, 1000 },
        { vk::DescriptorType::eStorageBufferDynamic, 1000 },
        { vk::DescriptorType::eInputAttachment, 1000 },
    };
    imGuiDescPool = vkb::getDevice()->createDescriptorPoolUnique(
        vk::DescriptorPoolCreateInfo(
            vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
            1000 * poolSizes.size(),
            poolSizes
        )
    );

    // Init ImGui for Vulkan
    ImGui_ImplGlfw_InitForVulkan(vkb::getSwapchain().getGlfwWindow(), true);
    ImGui_ImplVulkan_InitInfo igInfo{};
    igInfo.Instance = *vkb::getInstance();
    igInfo.PhysicalDevice = vkb::getPhysicalDevice().physicalDevice;
    igInfo.Device = *vkb::getDevice();
    igInfo.QueueFamily = vkb::getDevice().getQueueFamily(vkb::QueueType::graphics);
    igInfo.Queue = vkb::getDevice().getQueues(igInfo.QueueFamily).back();
    igInfo.PipelineCache = {};
    igInfo.DescriptorPool = *imGuiDescPool;
    igInfo.Allocator = nullptr;
    igInfo.MinImageCount = vkb::getSwapchain().getFrameCount();
    igInfo.ImageCount = vkb::getSwapchain().getFrameCount();
    ImGui_ImplVulkan_Init(&igInfo, *renderPass);

    // Upload fonts texture
    auto oneTimeCmdBuf = vkb::getDevice().createGraphicsCommandBuffer();
    oneTimeCmdBuf->begin(vk::CommandBufferBeginInfo());
    ImGui_ImplVulkan_CreateFontsTexture(*oneTimeCmdBuf);
    oneTimeCmdBuf->end();
    vkb::getDevice().executeGraphicsCommandBufferSynchronously(*oneTimeCmdBuf);

    // Create dummy pipeline
    vkb::ShaderProgram program(TRC_SHADER_DIR"/empty_vert.spv",
                               TRC_SHADER_DIR"/empty_frag.spv");
    auto layout = trc::makePipelineLayout({}, {});
    auto pipeline = trc::GraphicsPipelineBuilder::create()
        .setProgram(program)
        .addViewport({})
        .addScissorRect({})
        .addColorBlendAttachment(trc::DEFAULT_COLOR_BLEND_ATTACHMENT_DISABLED)
        .setColorBlending({}, false, vk::LogicOp::eOr, {})
        .addDynamicState(vk::DynamicState::eViewport)
        .addDynamicState(vk::DynamicState::eScissor)
        .build(*vkb::getDevice(), *layout, *renderPass, 0);
    trc::makeGraphicsPipeline(IMGUI_PIPELINE_INDEX, std::move(layout), std::move(pipeline));
}

trc::experimental::ImGuiRenderStage::~ImGuiRenderStage()
{
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
}



trc::experimental::ImGuiRenderPass::ImGuiRenderPass()
    :
    RenderPass(
        []() -> vk::UniqueRenderPass
        {
            vk::AttachmentReference colorRef(0, vk::ImageLayout::eColorAttachmentOptimal);

            return vkb::getDevice()->createRenderPassUnique(
                vk::RenderPassCreateInfo(
                    {},
                    std::vector<vk::AttachmentDescription>{
                        vk::AttachmentDescription(
                            {},
                            vkb::getSwapchain().getImageFormat(),
                            vk::SampleCountFlagBits::e1,
                            vk::AttachmentLoadOp::eLoad, vk::AttachmentStoreOp::eStore,
                            vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare,
                            vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::ePresentSrcKHR
                        )
                    },
                    std::vector<vk::SubpassDescription>{
                        vk::SubpassDescription(
                            {}, vk::PipelineBindPoint::eGraphics,
                            0, nullptr, // input attachments
                            1, &colorRef,
                            nullptr, // resolve attachments
                            nullptr, // depth attachments
                            0, nullptr // preserve attachments
                        ),
                    },
                    std::vector<vk::SubpassDependency>{
                        vk::SubpassDependency(
                            VK_SUBPASS_EXTERNAL, 0,
                            vk::PipelineStageFlagBits::eAllGraphics,
                            vk::PipelineStageFlagBits::eAllGraphics,
                            vk::AccessFlagBits::eColorAttachmentWrite,
                            vk::AccessFlagBits::eInputAttachmentRead,
                            vk::DependencyFlagBits::eByRegion
                        )
                    }
                )
            );
        }(),
        1 // subpass count
    ),
    framebuffers(
        vkb::getSwapchain(),
        [this](ui32 imageIndex) -> vk::UniqueFramebuffer
        {
            const vk::ImageView imageView = vkb::getSwapchain().getImageView(imageIndex);
            const vk::Extent2D imageSize = vkb::getSwapchain().getImageExtent();

            return vkb::getDevice()->createFramebufferUnique(
                vk::FramebufferCreateInfo(
                    {},
                    *renderPass,
                    1, &imageView,
                    imageSize.width, imageSize.height, 1
                )
            );
        })
{
}

void trc::experimental::ImGuiRenderPass::begin(
    vk::CommandBuffer cmdBuf,
    vk::SubpassContents subpassContents)
{
    // Bring swapchain image into eColorAttachmentOptimal layout. The final
    // lighting pass brings it into ePresentSrcKHR.
    cmdBuf.pipelineBarrier(
        vk::PipelineStageFlagBits::eAllGraphics,
        vk::PipelineStageFlagBits::eAllGraphics,
        vk::DependencyFlagBits::eByRegion,
        {}, // memory barriers
        {}, // buffer memory barriers
        vk::ImageMemoryBarrier(
            {}, {},
            vk::ImageLayout::ePresentSrcKHR, vk::ImageLayout::eColorAttachmentOptimal,
            0, 0,
            vkb::getSwapchain().getImage(vkb::getSwapchain().getCurrentFrame()),
            vkb::DEFAULT_SUBRES_RANGE
        )
    );

    cmdBuf.beginRenderPass(
        vk::RenderPassBeginInfo(
            *renderPass,
            **framebuffers,
            { { 0, 0 }, vkb::getSwapchain().getImageExtent() },
            {}
        ),
        subpassContents
    );
}

void trc::experimental::ImGuiRenderPass::end(vk::CommandBuffer cmdBuf)
{
    // Render imgui stuff here. This is good because:
    //  - I don't have multiple subpasses in this renderpass
    //  - I don't need to attach an imgui-draw-function to every single scene
    //  - I make sure that everything else that's attached to this pass/stage
    //    (for whatever reason) is executed before the imgui render
    ig::EndFrame();
    ig::Render();
    ImGui_ImplVulkan_RenderDrawData(ig::GetDrawData(), cmdBuf, VK_NULL_HANDLE);

    cmdBuf.endRenderPass();
}



void trc::experimental::initImgui(Renderer& renderer)
{
    static bool _init{ false };
    if (_init) return;
    _init = true;

    // Replace vkb's event callbacks
    auto window = vkb::getSwapchain().getGlfwWindow();
    vkbCharCallback = glfwSetCharCallback(window, [](GLFWwindow* window, unsigned int codepoint) {
        if (!ig::GetIO().WantCaptureKeyboard) {
            vkbCharCallback(window, codepoint);
        }
    });
    vkbKeyCallback = glfwSetKeyCallback(window,
        [](GLFWwindow* window, int key, int scancode, int action, int mods) {
            if (!ig::GetIO().WantCaptureKeyboard) {
                vkbKeyCallback(window, key, scancode, action, mods);
            }
        }
    );
    vkbMouseButtonCallback = glfwSetMouseButtonCallback(window,
        [](GLFWwindow* window, int button, int action, int mods) {
            if (!ig::GetIO().WantCaptureMouse) {
                vkbMouseButtonCallback(window, button, action, mods);
            }
        }
    );
    vkbScrollCallback = glfwSetScrollCallback(window,
        [](GLFWwindow* window, double xOffset, double yOffset) {
            if (!ig::GetIO().WantCaptureMouse) {
                vkbScrollCallback(window, xOffset, yOffset);
            }
        }
    );

    trc::RenderStageType::create(IMGUI_RENDER_STAGE_TYPE_ID, ImGuiRenderPass::NUM_SUBPASSES);
    const auto [stageId, stage] = trc::RenderStage::createAtNextIndex<ImGuiRenderStage>();
    renderer.enableRenderStageType(IMGUI_RENDER_STAGE_TYPE_ID, IMGUI_RENDER_STAGE_TYPE_PRIO);
    renderer.addRenderStage(IMGUI_RENDER_STAGE_TYPE_ID, stage);

    // Recreate the imgui stage on swapchain recreate
    vkb::on<vkb::SwapchainRecreateEvent>([stageId=stageId](auto&&) {
        trc::RenderStage::replace<ImGuiRenderStage>(stageId);
    });
}

void trc::experimental::beginImgui()
{
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ig::NewFrame();
}



void trc::experimental::ImGuiRoot::draw()
{
    for (auto& el : elements) {
        el->drawImGui();
    }
}
