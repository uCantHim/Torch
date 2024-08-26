#include "trc/ImguiIntegration.h"

#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

#include <trc/PipelineDefinitions.h>
#include <trc/TorchRenderStages.h>
#include <trc/base/Barriers.h>
#include <trc/base/Logging.h>
#include <trc/base/event/Event.h>
#include <trc/core/PipelineBuilder.h>
#include <trc/core/RenderPipelineTasks.h>

namespace ig = ImGui;



namespace trc::imgui
{

vk::UniqueDescriptorPool imguiDescPool;
bool imguiInitialized{ false };
bool imguiHasBegun{ false };

struct CallbackStorage
{
    GLFWwindowfocusfun torchCallbackWindowFocus;
    GLFWcursorposfun   torchCallbackCursorPos;
    GLFWcursorenterfun torchCallbackCursorEnter;
    GLFWmousebuttonfun torchCallbackMouseButton;
    GLFWscrollfun      torchCallbackScroll;
    GLFWkeyfun         torchCallbackKey;
    GLFWcharfun        torchCallbackChar;
    GLFWmonitorfun     torchCallbackMonitor;
};

std::unordered_map<GLFWwindow*, CallbackStorage> callbackStorages;

#define replace_cb(_window_, _name_, _device_) \
    glfwSet ## _name_ ## Callback( \
        _window_, \
        [](GLFWwindow* window, auto... args){ \
            if (!ig::GetIO().WantCapture ## _device_) { \
                auto& storage = callbackStorages.at(window); \
                auto& cb = storage.torchCallback ## _name_; \
                if (cb != nullptr) cb(window, args...); \
            } \
            else { \
                ImGui_ImplGlfw_ ## _name_ ## Callback(window, args...); \
            } \
        } \
    )

void initCallbacks(GLFWwindow* window)
{
    auto [it, success] = callbackStorages.try_emplace(window);
    if (!success) {
        throw std::runtime_error("Don't call initCallbacks multiple times for the same window!");
    }

    it->second = CallbackStorage{
        .torchCallbackWindowFocus = replace_cb(window, WindowFocus, Mouse),
        .torchCallbackCursorPos = replace_cb(window, CursorPos, Mouse),
        .torchCallbackCursorEnter = replace_cb(window, CursorEnter, Mouse),
        .torchCallbackMouseButton = replace_cb(window, MouseButton, Mouse),
        .torchCallbackScroll = replace_cb(window, Scroll, Mouse),
        .torchCallbackKey = replace_cb(window, Key, Keyboard),
        .torchCallbackChar = replace_cb(window, Char, Keyboard),
        // Don't set the monitor callback because I'm too lazy to deal with
        // that right now.
        .torchCallbackMonitor{},
    };
}

void restoreCallbacks(GLFWwindow* window)
{
    auto& storage = callbackStorages.at(window);
    glfwSetWindowFocusCallback(window, storage.torchCallbackWindowFocus);
    glfwSetCursorEnterCallback(window, storage.torchCallbackCursorEnter);
    glfwSetCursorPosCallback(window, storage.torchCallbackCursorPos);
    glfwSetMouseButtonCallback(window, storage.torchCallbackMouseButton);
    glfwSetScrollCallback(window, storage.torchCallbackScroll);
    glfwSetKeyCallback(window, storage.torchCallbackKey);
    glfwSetCharCallback(window, storage.torchCallbackChar);
    //glfwSetMonitorCallback(storage.torchCallbackMonitor);

    callbackStorages.erase(window);
}



void initImgui(Window& window)
{
    auto& instance = window.getInstance();
    auto& device = window.getDevice();

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

            auto imageFormat = window.getImageFormat();
            vk::PipelineRenderingCreateInfo pipelineRenderingInfo{
                0x00,
                imageFormat,
                vk::Format::eUndefined,  // depth attachment format
                vk::Format::eUndefined,  // stencil attachment format
            };

            // Init ImGui for Vulkan
            ImGui_ImplVulkan_InitInfo igInfo{
                .Instance                    = instance.getVulkanInstance(),
                .PhysicalDevice              = *device.getPhysicalDevice(),
                .Device                      = *device,
                .QueueFamily                 = family,
                .Queue                       = *queue,
                .DescriptorPool              = *imguiDescPool,
                .RenderPass                  = VK_NULL_HANDLE,
                .MinImageCount               = window.getFrameCount(),
                .ImageCount                  = window.getFrameCount(),
                .MSAASamples                 = VK_SAMPLE_COUNT_1_BIT,
                .PipelineCache               = VK_NULL_HANDLE,
                .Subpass                     = 0,
                .UseDynamicRendering         = true,
                .PipelineRenderingCreateInfo = pipelineRenderingInfo,
                .Allocator                   = nullptr,
                .CheckVkResultFn             = nullptr,
                .MinAllocationSize           = 1024 * 1024,
            };
            ImGui_ImplVulkan_Init(&igInfo);
        }
        catch (const QueueReservedError& err)
        {
            throw std::runtime_error("Unable to reserve graphics queue for ImGui: "
                                     + std::string(err.what()));
        }

        // Upload fonts texture
        if (!ImGui_ImplVulkan_CreateFontsTexture()) {
            log::warn << log::here() << ": Unable to create fonts texture:"
                                        " ImGui_ImplVulkan_CreateFontsTexture returned false.";
        }

        imguiInitialized = true;
    }

    // Initialize window-specific stuff
    ImGui_ImplGlfw_InitForVulkan(window.getGlfwWindow(), false);
}

void terminateImgui()
{
    if (imguiInitialized)
    {
        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();

        imguiDescPool.reset();

        imguiInitialized = false;
    }
}

void beginImguiFrame()
{
    if (!imguiHasBegun)
    {
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ig::NewFrame();

        imguiHasBegun = true;
    }
}

void endImguiFrame()
{
    if (imguiHasBegun)
    {
        ig::EndFrame();
        imguiHasBegun = false;
    }
}

auto buildImguiRenderPlugin(Window& window) -> PluginBuilder
{
    return [&window](auto&&) -> u_ptr<RenderPlugin> {
        return std::make_unique<ImguiRenderPlugin>(window);
    };
}



ImguiRenderPlugin::ImguiRenderPlugin(Window& _window)
    :
    window(_window.getGlfwWindow())
{
    initImgui(_window);
    initCallbacks(window);
}

ImguiRenderPlugin::~ImguiRenderPlugin() noexcept
{
    restoreCallbacks(window);
    if (callbackStorages.empty())
    {
        // No ImguiRenderPass instances exist anymore. Terminate now for
        // safety.
        terminateImgui();
    }
}

void ImguiRenderPlugin::defineRenderStages(RenderGraph& graph)
{
    graph.createOrdering(renderTargetImageInitStage, imguiRenderStage);
    graph.createOrdering(finalLightingRenderStage, imguiRenderStage);
    graph.createOrdering(finalCompositingRenderStage, imguiRenderStage);
    graph.createOrdering(imguiRenderStage, renderTargetImageFinalizeStage);
}

void ImguiRenderPlugin::defineResources(ResourceConfig& /*resources*/)
{
    // Nothing to do - ImGui holds all of its resources.
}

auto ImguiRenderPlugin::createGlobalResources(RenderPipelineContext& /*ctx*/)
    -> u_ptr<GlobalResources>
{
    return std::make_unique<ImguiDrawResources>();
}



void ImguiRenderPlugin::ImguiDrawResources::registerResources(ResourceStorage& /*resources*/)
{
    // Nothing to do - ImGui holds all of its resources.
}

void ImguiRenderPlugin::ImguiDrawResources::hostUpdate(RenderPipelineContext& /*ctx*/)
{
    // Nothing to do
}

void ImguiRenderPlugin::ImguiDrawResources::createTasks(GlobalUpdateTaskQueue& queue)
{
    queue.spawnTask(imguiRenderStage, dispatchImguiCommands);
}

void ImguiRenderPlugin::ImguiDrawResources::dispatchImguiCommands(
    vk::CommandBuffer cmdBuf,
    GlobalUpdateContext& ctx)
{
    if (!imguiHasBegun)
    {
        log::debug << "Imgui frame was never started via `beginImguiFrame()` before"
                      " the frame was submitted for rendering. No commands will be"
                      " executed.";
        return;
    }
    endImguiFrame();

    auto& target = ctx.renderTargetImage();

    ctx.deps().consume(ImageAccess{
        target.image,
        vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1),
        vk::PipelineStageFlagBits2::eColorAttachmentOutput,
        vk::AccessFlagBits2::eColorAttachmentWrite,
        vk::ImageLayout::eColorAttachmentOptimal,
    });

    // Render to the current render target image
    vk::RenderingAttachmentInfo colorAttachment{
        target.imageView, vk::ImageLayout::eColorAttachmentOptimal,  // color attachment image
        vk::ResolveModeFlagBits::eNone, VK_NULL_HANDLE, vk::ImageLayout::eUndefined,  // resolve image
        vk::AttachmentLoadOp::eLoad, vk::AttachmentStoreOp::eStore,
        vk::ClearValue{}
    };
    vk::RenderingInfo renderInfo{
        {},
        { { 0, 0 }, { target.size.x, target.size.y } },
        1, 0,
        colorAttachment,
        nullptr, nullptr  // depth an stencil attachments
    };

    // Collect and submit ImGui draw commands
    cmdBuf.beginRendering(renderInfo);
    ig::Render();
    ImGui_ImplVulkan_RenderDrawData(ig::GetDrawData(), cmdBuf, VK_NULL_HANDLE);
    cmdBuf.endRendering();

    ctx.deps().produce(ImageAccess{
        target.image,
        vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1),
        vk::PipelineStageFlagBits2::eColorAttachmentOutput,
        vk::AccessFlagBits2::eColorAttachmentWrite,
        vk::ImageLayout::eColorAttachmentOptimal,
    });
}

} // namespace trc::imgui
