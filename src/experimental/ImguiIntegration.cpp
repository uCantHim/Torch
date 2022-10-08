#include "trc/experimental/ImguiIntegration.h"

#include <unordered_map>

#include "trc/base//Barriers.h"
#include "trc/base//event/Event.h"
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

#include "trc/core/RenderLayout.h"
#include "trc/core/PipelineBuilder.h"
#include "trc/Torch.h"
#include "trc/TorchResources.h"
#include "trc/PipelineDefinitions.h"



namespace
{
    vk::UniqueDescriptorPool imguiDescPool;
    bool imguiInitialized{ false };

    bool imguiHasBegun{ false };
} // anonymous namespace



auto trc::experimental::imgui::initImgui(Window& window, RenderLayout& layout)
    -> trc::u_ptr<ImguiRenderPass>
{
    auto& device = window.getDevice();
    auto& swapchain = window.getSwapchain();

    auto renderPass = std::make_unique<ImguiRenderPass>(swapchain);
    layout.addPass(imguiRenderStage, *renderPass);

    // Initialize global imgui stuff
    if (!imguiInitialized)
    {
        // Standard ImGui initialization stuff
        IMGUI_CHECKVERSION();
        ig::SetCurrentContext(ig::CreateContext());
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
        imguiDescPool = device->createDescriptorPoolUnique(
            vk::DescriptorPoolCreateInfo(
                vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
                1000 * poolSizes.size(),
                poolSizes
            )
        );

        try {
            auto [queue, family] = device.getQueueManager().getAnyQueue(QueueType::graphics);
            device.getQueueManager().reserveQueue(queue);

            // Init ImGui for Vulkan
            ImGui_ImplVulkan_InitInfo igInfo{};
            igInfo.Instance = window.getInstance().getVulkanInstance(),
            igInfo.PhysicalDevice = *device.getPhysicalDevice();
            igInfo.Device = *device;
            igInfo.QueueFamily = family;
            igInfo.Queue = *queue;
            igInfo.PipelineCache = {};
            igInfo.DescriptorPool = *imguiDescPool;
            igInfo.Allocator = nullptr;
            igInfo.MinImageCount = swapchain.getFrameCount();
            igInfo.ImageCount = swapchain.getFrameCount();
            ImGui_ImplVulkan_Init(&igInfo, **renderPass);
        }
        catch (const QueueReservedError& err)
        {
            throw std::runtime_error("Unable to reserve graphics queue for ImGui: "
                                     + std::string(err.what()));
        }

        // Upload fonts texture
        device.executeCommands(QueueType::graphics, [](auto cmdBuf) {
            ImGui_ImplVulkan_CreateFontsTexture(cmdBuf);
        });

        imguiInitialized = true;
    }

    // Initialize window-specific stuff
    ImGui_ImplGlfw_InitForVulkan(swapchain.getGlfwWindow(), true);

    return renderPass;
}

void trc::experimental::imgui::terminateImgui()
{
    if (imguiInitialized)
    {
        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();

        imguiDescPool.reset();

        imguiInitialized = false;
    }
}

void trc::experimental::imgui::beginImguiFrame()
{
    if (!imguiHasBegun)
    {
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ig::NewFrame();

        imguiHasBegun = true;
    }
}



trc::experimental::imgui::ImguiRenderPass::ImguiRenderPass(const Swapchain& swapchain)
    :
    RenderPass(
        [&swapchain]() -> vk::UniqueRenderPass
        {
            vk::AttachmentReference colorRef(0, vk::ImageLayout::eColorAttachmentOptimal);

            std::vector<vk::AttachmentDescription> attachments{
                vk::AttachmentDescription(
                    {},
                    swapchain.getImageFormat(),
                    vk::SampleCountFlagBits::e1,
                    vk::AttachmentLoadOp::eLoad, vk::AttachmentStoreOp::eStore,
                    vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare,
                    vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::ePresentSrcKHR
                )
            };
            std::vector<vk::SubpassDescription> subpasses{
                vk::SubpassDescription(
                    {}, vk::PipelineBindPoint::eGraphics,
                    0, nullptr, // input attachments
                    1, &colorRef,
                    nullptr, // resolve attachments
                    nullptr, // depth attachments
                    0, nullptr // preserve attachments
                ),
            };
            std::vector<vk::SubpassDependency> dependencies{
                vk::SubpassDependency(
                    VK_SUBPASS_EXTERNAL, 0,
                    vk::PipelineStageFlagBits::eAllGraphics,
                    vk::PipelineStageFlagBits::eAllGraphics,
                    vk::AccessFlagBits::eColorAttachmentWrite,
                    vk::AccessFlagBits::eInputAttachmentRead,
                    vk::DependencyFlagBits::eByRegion
                )
            };

            return swapchain.device->createRenderPassUnique(
                vk::RenderPassCreateInfo({}, attachments, subpasses, dependencies)
            );
        }(),
        1 // subpass count
    ),
    swapchain(swapchain),
    // Create pipeline
    imguiPipelineLayout(trc::makePipelineLayout(swapchain.device, {}, {})),
    imguiPipeline(trc::buildGraphicsPipeline()
        .setProgram(internal::loadShader(ShaderPath("/empty.vert")),
                    internal::loadShader(ShaderPath("/empty.frag")))
        .addViewport({})
        .addScissorRect({})
        .addColorBlendAttachment(trc::DEFAULT_COLOR_BLEND_ATTACHMENT_DISABLED)
        .setColorBlending({}, false, vk::LogicOp::eOr, {})
        .addDynamicState(vk::DynamicState::eViewport)
        .addDynamicState(vk::DynamicState::eScissor)
        .build(swapchain.device, imguiPipelineLayout, *renderPass, 0)
    ),
    framebuffers(swapchain)
{
    auto window = swapchain.getGlfwWindow();
    auto it = callbackStorages.find(window);
    if (it != callbackStorages.end()) {
        throw std::runtime_error("Don't create multiple ImguiRenderPasses for the same swapchain!");
    }
    else {
        it = callbackStorages.try_emplace(window).first;
    }

    // Replace trc's event callbacks
    auto& storage = it->second;

    storage.trcCharCallback = glfwSetCharCallback(window,
        [](GLFWwindow* window, unsigned int codepoint) {
            if (!ig::GetIO().WantCaptureKeyboard) {
                callbackStorages.at(window).trcCharCallback(window, codepoint);
            }
        }
    );
    storage.trcKeyCallback = glfwSetKeyCallback(window,
        [](GLFWwindow* window, int key, int scancode, int action, int mods) {
            if (!ig::GetIO().WantCaptureKeyboard) {
                callbackStorages.at(window).trcKeyCallback(window, key, scancode, action, mods);
            }
        }
    );
    storage.trcMouseButtonCallback = glfwSetMouseButtonCallback(window,
        [](GLFWwindow* window, int button, int action, int mods) {
            if (!ig::GetIO().WantCaptureMouse) {
                callbackStorages.at(window).trcMouseButtonCallback(window, button, action, mods);
            }
        }
    );
    storage.trcScrollCallback = glfwSetScrollCallback(window,
        [](GLFWwindow* window, double xOffset, double yOffset) {
            if (!ig::GetIO().WantCaptureMouse) {
                callbackStorages.at(window).trcScrollCallback(window, xOffset, yOffset);
            }
        }
    );

    createFramebuffers();
    swapchainRecreateListener = on<SwapchainRecreateEvent>([this](auto& e) {
        if (e.swapchain != &this->swapchain) return;
        createFramebuffers();
    });
}

trc::experimental::imgui::ImguiRenderPass::~ImguiRenderPass()
{
    swapchain.device->waitIdle();

    auto window = swapchain.getGlfwWindow();
    auto& storage = callbackStorages.at(window);

    // Replace trc's event callbacks
    glfwSetCharCallback(window, storage.trcCharCallback);
    glfwSetKeyCallback(window, storage.trcKeyCallback);
    glfwSetMouseButtonCallback(window, storage.trcMouseButtonCallback);
    glfwSetScrollCallback(window, storage.trcScrollCallback);

    callbackStorages.erase(window);
    if (callbackStorages.empty())
    {
        // No ImguiRenderPass instances exist anymore. Terminate now for
        // safety.
        terminateImgui();
    }
}

void trc::experimental::imgui::ImguiRenderPass::begin(
    vk::CommandBuffer cmdBuf,
    vk::SubpassContents subpassContents,
    FrameRenderState&)
{
    // Bring swapchain image into eColorAttachmentOptimal layout. The final
    // lighting pass brings it into ePresentSrcKHR.
    imageMemoryBarrier(
        cmdBuf,
        swapchain.getImage(swapchain.getCurrentFrame()),
        vk::ImageLayout::ePresentSrcKHR, vk::ImageLayout::eColorAttachmentOptimal,
        vk::PipelineStageFlagBits::eComputeShader,
        vk::PipelineStageFlagBits::eColorAttachmentOutput,
        vk::AccessFlagBits::eMemoryRead | vk::AccessFlagBits::eMemoryWrite,
        vk::AccessFlagBits::eMemoryRead | vk::AccessFlagBits::eMemoryWrite,
        DEFAULT_SUBRES_RANGE
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

    // If imgui hasn't been begun yet, do it now.
    if (!imguiHasBegun) {
        beginImguiFrame();
    }
}

void trc::experimental::imgui::ImguiRenderPass::end(vk::CommandBuffer cmdBuf)
{
    ig::EndFrame();
    ig::Render();
    ImGui_ImplVulkan_RenderDrawData(ig::GetDrawData(), cmdBuf, VK_NULL_HANDLE);
    imguiHasBegun = false;

    cmdBuf.endRenderPass();
}

void trc::experimental::imgui::ImguiRenderPass::createFramebuffers()
{
    framebuffers = {
        swapchain,
        [this](ui32 imageIndex) -> vk::UniqueFramebuffer
        {
            const vk::ImageView imageView = swapchain.getImageView(imageIndex);
            const vk::Extent2D imageSize = swapchain.getImageExtent();

            return swapchain.device->createFramebufferUnique(
                vk::FramebufferCreateInfo(
                    {},
                    *renderPass,
                    1, &imageView,
                    imageSize.width, imageSize.height, 1
                )
            );
        }
    };
}
