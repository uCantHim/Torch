#include "experimental/ImguiIntegration.h"

#include <unordered_map>

#include <vkb/event/Event.h>

#include "PipelineBuilder.h"



namespace
{
    trc::Pipeline::ID imguiPipelineId;
    bool imguiInitialized{ false };
} // anonymous namespace



auto trc::experimental::getImguiRenderStageType() -> RenderStageType::ID
{
    static RenderStageType::ID imguiStageId = RenderStageType::createAtNextIndex(1).first;
    return imguiStageId;
}

auto trc::experimental::getImguiRenderPass(const vkb::Swapchain& swapchain) -> RenderPass::ID
{
    static std::unordered_map<const vkb::Swapchain*, RenderPass::ID> passesPerSwapchain;

    auto it = passesPerSwapchain.find(&swapchain);
    if (it == passesPerSwapchain.end())
    {
        it = passesPerSwapchain.try_emplace(
            &swapchain,
            RenderPass::createAtNextIndex<ImGuiRenderPass>(swapchain).first
        ).first;
    }

    return it->second;
}

auto trc::experimental::getImguiPipeline() -> Pipeline::ID
{
    if (!imguiInitialized) {
        throw std::runtime_error("In getImguiPipeline(): ImGui has not been initialized."
                                 " Call initImgui first!");
    }

    return imguiPipelineId;
}



trc::experimental::ImGuiRenderPass::ImGuiRenderPass(const vkb::Swapchain& swapchain)
    :
    RenderPass(
        [&swapchain]() -> vk::UniqueRenderPass
        {
            vk::AttachmentReference colorRef(0, vk::ImageLayout::eColorAttachmentOptimal);

            return vkb::getDevice()->createRenderPassUnique(
                vk::RenderPassCreateInfo(
                    {},
                    std::vector<vk::AttachmentDescription>{
                        vk::AttachmentDescription(
                            {},
                            swapchain.getImageFormat(),
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
    swapchain(swapchain),
    framebuffers(
        swapchain,
        [this, &swapchain](ui32 imageIndex) -> vk::UniqueFramebuffer
        {
            const vk::ImageView imageView = swapchain.getImageView(imageIndex);
            const vk::Extent2D imageSize = swapchain.getImageExtent();

            return vkb::getDevice()->createFramebufferUnique(
                vk::FramebufferCreateInfo(
                    {},
                    *renderPass,
                    1, &imageView,
                    imageSize.width, imageSize.height, 1
                )
            );
        }
    )
{
    auto window = vkb::getSwapchain().getGlfwWindow();
    auto it = callbackStorages.find(window);
    if (it == callbackStorages.end()) {
        it = callbackStorages.try_emplace(window).first;
    }

    auto& storage = it->second;

    // Replace vkb's event callbacks
    storage.vkbCharCallback = glfwSetCharCallback(window,
        [](GLFWwindow* window, unsigned int codepoint) {
            if (!ig::GetIO().WantCaptureKeyboard) {
                callbackStorages.at(window).vkbCharCallback(window, codepoint);
            }
        }
    );
    storage.vkbKeyCallback = glfwSetKeyCallback(window,
        [](GLFWwindow* window, int key, int scancode, int action, int mods) {
            if (!ig::GetIO().WantCaptureKeyboard) {
                callbackStorages.at(window).vkbKeyCallback(window, key, scancode, action, mods);
            }
        }
    );
    storage.vkbMouseButtonCallback = glfwSetMouseButtonCallback(window,
        [](GLFWwindow* window, int button, int action, int mods) {
            if (!ig::GetIO().WantCaptureMouse) {
                callbackStorages.at(window).vkbMouseButtonCallback(window, button, action, mods);
            }
        }
    );
    storage.vkbScrollCallback = glfwSetScrollCallback(window,
        [](GLFWwindow* window, double xOffset, double yOffset) {
            if (!ig::GetIO().WantCaptureMouse) {
                callbackStorages.at(window).vkbScrollCallback(window, xOffset, yOffset);
            }
        }
    );
}

trc::experimental::ImGuiRenderPass::~ImGuiRenderPass()
{
    auto window = vkb::getSwapchain().getGlfwWindow();
    auto& storage = callbackStorages.at(window);

    // Replace vkb's event callbacks
    glfwSetCharCallback(window, storage.vkbCharCallback);
    glfwSetKeyCallback(window, storage.vkbKeyCallback);
    glfwSetMouseButtonCallback(window, storage.vkbMouseButtonCallback);
    glfwSetScrollCallback(window, storage.vkbScrollCallback);

    callbackStorages.erase(window);
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
            swapchain.getImage(swapchain.getCurrentFrame()),
            vkb::DEFAULT_SUBRES_RANGE
        )
    );

    cmdBuf.beginRenderPass(
        vk::RenderPassBeginInfo(
            *renderPass,
            **framebuffers,
            { { 0, 0 }, swapchain.getImageExtent() },
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
    beginImgui();

    ig::EndFrame();
    ig::Render();
    ImGui_ImplVulkan_RenderDrawData(ig::GetDrawData(), cmdBuf, VK_NULL_HANDLE);

    cmdBuf.endRenderPass();
}



void trc::experimental::initImgui(
    const vkb::Device& device,
    Renderer& renderer,
    const vkb::Swapchain& swapchain)
{
    auto& graph = renderer.getRenderGraph();
    graph.after(trc::RenderStageTypes::getDeferred(), getImguiRenderStageType());
    graph.addPass(getImguiRenderStageType(), getImguiRenderPass(swapchain));

    // Recreate the imgui pass on swapchain recreate
    vkb::on<vkb::SwapchainRecreateEvent>([&swapchain](const auto& e) {
        if (e.swapchain == &swapchain) {
            RenderPass::replace<ImGuiRenderPass>(getImguiRenderPass(swapchain), swapchain);
        }
    });

    [[maybe_unused]]
    static bool _init_once = [&device, &swapchain]
    {
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
        static auto imGuiDescPool = device->createDescriptorPoolUnique(
            vk::DescriptorPoolCreateInfo(
                vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
                1000 * poolSizes.size(),
                poolSizes
            )
        );

        RenderPass& renderPass = RenderPass::at(getImguiRenderPass(swapchain));

        // Init ImGui for Vulkan
        ImGui_ImplGlfw_InitForVulkan(vkb::getSwapchain().getGlfwWindow(), true);
        ImGui_ImplVulkan_InitInfo igInfo{};
        igInfo.Instance = *vkb::getInstance();
        igInfo.PhysicalDevice = vkb::getPhysicalDevice().physicalDevice;
        igInfo.Device = *device;
        igInfo.QueueFamily = device.getQueueFamily(vkb::QueueType::graphics);
        igInfo.Queue = device.getQueues(igInfo.QueueFamily).back();
        igInfo.PipelineCache = {};
        igInfo.DescriptorPool = *imGuiDescPool;
        igInfo.Allocator = nullptr;
        igInfo.MinImageCount = vkb::getSwapchain().getFrameCount();
        igInfo.ImageCount = vkb::getSwapchain().getFrameCount();
        ImGui_ImplVulkan_Init(&igInfo, *renderPass);

        // Upload fonts texture
        auto oneTimeCmdBuf = device.createGraphicsCommandBuffer();
        oneTimeCmdBuf->begin(vk::CommandBufferBeginInfo());
        ImGui_ImplVulkan_CreateFontsTexture(*oneTimeCmdBuf);
        oneTimeCmdBuf->end();
        device.executeGraphicsCommandBufferSynchronously(*oneTimeCmdBuf);

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
            .build(*device, *layout, *renderPass, 0);

        imguiPipelineId = trc::Pipeline::createAtNextIndex(
            std::move(layout),
            std::move(pipeline),
            vk::PipelineBindPoint::eGraphics
        ).first;

        imguiInitialized = true;
        return true;
    }();
}

void trc::experimental::terminateImgui()
{
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
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
