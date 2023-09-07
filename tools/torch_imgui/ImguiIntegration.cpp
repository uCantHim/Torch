#include "trc/ImguiIntegration.h"

#include <unordered_map>

#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

#include "trc/PipelineDefinitions.h"
#include "trc/Torch.h"
#include "trc/TorchRenderStages.h"
#include "trc/base/Barriers.h"
#include "trc/base/event/Event.h"
#include "trc/core/PipelineBuilder.h"

namespace ig = ImGui;



namespace
{
    vk::UniqueDescriptorPool imguiDescPool;
    bool imguiInitialized{ false };

    bool imguiHasBegun{ false };
} // anonymous namespace

auto trc::imgui::initImgui(Window& window, RenderGraph& graph)
    -> trc::u_ptr<ImguiRenderPass>
{
    auto& device = window.getDevice();
    auto& swapchain = window.getSwapchain();

    auto renderPass = std::make_unique<ImguiRenderPass>(swapchain);
    graph.addPass(imguiRenderStage, *renderPass);

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
            ImGui_ImplVulkan_InitInfo igInfo{
                .Instance              = window.getInstance().getVulkanInstance(),
                .PhysicalDevice        = *device.getPhysicalDevice(),
                .Device                = *device,
                .QueueFamily           = family,
                .Queue                 = *queue,
                .PipelineCache         = VK_NULL_HANDLE,
                .DescriptorPool        = *imguiDescPool,
                .Subpass               = 0,
                .MinImageCount         = swapchain.getFrameCount(),
                .ImageCount            = swapchain.getFrameCount(),
                .MSAASamples           = VK_SAMPLE_COUNT_1_BIT,
                .UseDynamicRendering   = true,
                .ColorAttachmentFormat = VkFormat(swapchain.getImageFormat()),
                .Allocator             = nullptr,
                .CheckVkResultFn       = nullptr,
            };
            ImGui_ImplVulkan_Init(&igInfo, VK_NULL_HANDLE);
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

void trc::imgui::terminateImgui()
{
    if (imguiInitialized)
    {
        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();

        imguiDescPool.reset();

        imguiInitialized = false;
    }
}

void trc::imgui::beginImguiFrame()
{
    if (!imguiHasBegun)
    {
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ig::NewFrame();

        imguiHasBegun = true;
    }
}



trc::imgui::ImguiRenderPass::ImguiRenderPass(const Swapchain& swapchain)
    :
    RenderPass({}, 0),
    swapchain(swapchain)
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
}

trc::imgui::ImguiRenderPass::~ImguiRenderPass()
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

void trc::imgui::ImguiRenderPass::begin(
    vk::CommandBuffer cmdBuf,
    vk::SubpassContents /*subpassContents*/,
    FrameRenderState&)
{
    // Bring swapchain image into eColorAttachmentOptimal layout. The final
    // lighting pass brings it into ePresentSrcKHR.
    barrier(cmdBuf, vk::ImageMemoryBarrier2{
        vk::PipelineStageFlagBits2::eComputeShader,
        vk::AccessFlagBits2::eShaderStorageRead | vk::AccessFlagBits2::eShaderStorageWrite,
        vk::PipelineStageFlagBits2::eColorAttachmentOutput,
        vk::AccessFlagBits2::eColorAttachmentWrite,
        vk::ImageLayout::ePresentSrcKHR, vk::ImageLayout::eColorAttachmentOptimal,
        VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
        swapchain.getImage(swapchain.getCurrentFrame()),
        vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1)
    });

    vk::RenderingAttachmentInfo colorAttachment{
        swapchain.getImageView(swapchain.getCurrentFrame()),
        vk::ImageLayout::eColorAttachmentOptimal,
        vk::ResolveModeFlagBits::eNone, VK_NULL_HANDLE, vk::ImageLayout::eUndefined,
        vk::AttachmentLoadOp::eLoad, vk::AttachmentStoreOp::eStore,
        vk::ClearValue{}
    };
    vk::RenderingInfo renderInfo{
        {},
        { { 0, 0 }, swapchain.getImageExtent() },
        1, 0,
        colorAttachment,
        nullptr, nullptr  // depth an stencil attachments
    };

    cmdBuf.beginRendering(renderInfo);

    // If imgui hasn't been begun yet, do it now.
    if (!imguiHasBegun) {
        beginImguiFrame();
    }
}

void trc::imgui::ImguiRenderPass::end(vk::CommandBuffer cmdBuf)
{
    ig::EndFrame();
    ig::Render();
    ImGui_ImplVulkan_RenderDrawData(ig::GetDrawData(), cmdBuf, VK_NULL_HANDLE);
    imguiHasBegun = false;

    cmdBuf.endRendering();

    barrier(cmdBuf, vk::ImageMemoryBarrier2{
        vk::PipelineStageFlagBits2::eColorAttachmentOutput,
        vk::AccessFlagBits2::eColorAttachmentWrite,
        vk::PipelineStageFlagBits2::eHost,
        vk::AccessFlagBits2::eHostRead,
        vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::ePresentSrcKHR,
        VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
        swapchain.getImage(swapchain.getCurrentFrame()),
        vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1)
    });
}
