#pragma once

#include <vkb/Swapchain.h>
#include <imgui.h>

#include "core/Window.h"
#include "core/RenderStage.h"
#include "core/RenderPass.h"
#include "core/Pipeline.h"

namespace trc
{
    class RenderLayout;
}

namespace trc::experimental::imgui
{
    namespace ig = ImGui;

    class ImguiRenderPass;

    inline RenderStage imguiRenderStage{};

    /**
     * @brief Initialize imgui integration and set up render graph
     *
     * This function fully initializes imgui and sets a render graph up to
     * use imgui. You can call beginImguiFrame() and use ImGui functionality
     * after a call to this function.
     *
     * Call this function for each window on which you want to use imgui.
     *
     * @param Window& window The window on which to enable imgui
     * @param RenderLayout& layout Adds imgui render pass to the render
     *                             layout.
     */
    extern auto initImgui(Window& window, RenderLayout& layout) -> u_ptr<ImguiRenderPass>;

    extern void terminateImgui();

    /**
     * @brief Begin ImGui command recording
     *
     * ImguiRenderPass calls this function before executing registered
     * drawable functions if it hasn't been called before. If you only call
     * imgui stuff via scene-registered drawable functions, you don't ever
     * have to call this function.
     *
     * The Imgui frame ends when the ImguiRenderPass is executed. This
     * happens in Renderer::drawFrame (after the deferred lighting pass).
     *
     * You can issue Imgui draw commands between the begin and end of the
     * Imgui frame. If you want to, you can call beginImguiFrame() as the first
     * thing in your main loop.
     */
    extern void beginImguiFrame();

    /**
     * @brief RenderPass for Imgui
     */
    class ImguiRenderPass : public RenderPass
    {
    public:
        static constexpr ui32 NUM_SUBPASSES = 1;

        explicit ImguiRenderPass(const vkb::Swapchain& swapchain);
        ~ImguiRenderPass() override;

        void begin(vk::CommandBuffer cmdBuf, vk::SubpassContents, FrameRenderState&) override;
        void end(vk::CommandBuffer cmdBuf) override;

    private:
        struct CallbackStorage
        {
            GLFWcharfun vkbCharCallback;
            GLFWkeyfun vkbKeyCallback;
            GLFWmousebuttonfun vkbMouseButtonCallback;
            GLFWscrollfun vkbScrollCallback;
        };

        void createFramebuffers();

        static inline std::unordered_map<const GLFWwindow*, CallbackStorage> callbackStorages;

        const vkb::Swapchain& swapchain;
        PipelineLayout imguiPipelineLayout;
        Pipeline imguiPipeline;
        vkb::FrameSpecific<vk::UniqueFramebuffer> framebuffers;

        vkb::UniqueListenerId<vkb::SwapchainRecreateEvent> swapchainRecreateListener;
    };
} // namespace trc::experimental::imgui
