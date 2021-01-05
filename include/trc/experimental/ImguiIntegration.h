#pragma once

#include <vkb/basics/Swapchain.h>
#include <imgui.h>

#include "RenderStage.h"
#include "RenderPass.h"
#include "Pipeline.h"

namespace trc
{
    namespace ig = ImGui;

    class Renderer;
}

namespace trc::experimental::imgui
{
    extern auto getImguiRenderStageType() -> RenderStageType::ID;
    extern auto getImguiRenderPass(const vkb::Swapchain& swapchain) -> RenderPass::ID;
    extern auto getImguiPipeline() -> Pipeline::ID;

    /**
     * @brief Initialize imgui integration and set up renderer
     *
     * This function fully initializes imgui and sets up a renderer to use
     * imgui. You can call beginImguiFrame() and use ImGui functionality
     * after a call to this function.
     *
     * @param const vkb::Device& device
     * @param Renderer& renderer Adds imgui stage and render pass to the
     *                           renderer's graph.
     * @param const vkb::Swapchain& swapchain
     */
    extern void initImgui(const vkb::Device& device,
                          Renderer& renderer,
                          const vkb::Swapchain& swapchain);

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
     *
     * Renders to the swapchain, but after the final lighting pass. Meaning
     * it overwrites everything else.
     */
    class ImguiRenderPass : public RenderPass
    {
    public:
        static constexpr ui32 NUM_SUBPASSES = 1;

        explicit ImguiRenderPass(const vkb::Swapchain& swapchain);
        ~ImguiRenderPass() override;

        void begin(vk::CommandBuffer cmdBuf, vk::SubpassContents subpassContents) override;
        void end(vk::CommandBuffer cmdBuf) override;

    private:
        struct CallbackStorage
        {
            GLFWcharfun vkbCharCallback;
            GLFWkeyfun vkbKeyCallback;
            GLFWmousebuttonfun vkbMouseButtonCallback;
            GLFWscrollfun vkbScrollCallback;
        };

        static inline std::unordered_map<const GLFWwindow*, CallbackStorage> callbackStorages;

        const vkb::Swapchain& swapchain;
        vkb::FrameSpecificObject<vk::UniqueFramebuffer> framebuffers;
    };
} // namespace trc::experimental::imgui
