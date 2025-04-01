#pragma once

#include <imgui.h>

#include <trc/core/RenderGraph.h>
#include <trc/core/RenderPass.h>
#include <trc/core/RenderPlugin.h>
#include <trc/core/RenderStage.h>
#include <trc/core/Window.h>

namespace trc::imgui
{
    using namespace trc::basic_types;

    inline RenderStage imguiRenderStage = trc::makeRenderStage();

    namespace impl_callback
    {
        void cursorPos(Swapchain& sc, double x, double y);
        void windowFocus(Swapchain& sc, bool focused);
        void cursorEnter(Swapchain& sc, bool entered);
        void mouseButton(Swapchain& sc, MouseButton button, InputAction action, KeyModFlags mods);
        void scroll(Swapchain& sc, double xOff, double yOff);
        void key(Swapchain& sc, Key key, InputAction action, KeyModFlags mods);
        void charInput(Swapchain& sc, ui32 c);
        void monitor(GLFWmonitor* monitor, int event);
    } // namespace callback

    /**
     * @brief A configuration option.
     *
     * When an `ImguiRenderPlugin` is created for a specific window, that
     * window's event callbacks are overwritten with imgui-specific ones. These
     * callbacks dispatch all events to ImGui first, then, if ImGui allows it,
     * pass them along to the original Torch callbacks.
     *
     * If you don't want this behaviour, call this function before creating any
     * `ImguiRenderPlugin`. In this case, you can use the functions in the
     * `trc::imgui::impl_callback` namespace to call ImGui's backend-specific
     * event callbacks yourself.
     */
    void disableInsertingImguiEventCallbacks();

    /**
     * @brief Initialize ImGui for a window.
     *
     * Is called automatically when creating an ImguiRenderPlugin.
     *
     * @param Window& window The window on which to enable imgui
     */
    void initImgui(Window& window);

    /**
     * @brief Terminate ImGui.
     */
    void terminateImgui();

    /**
     * @brief Begin an ImGui frame.
     *
     * Torch's equivalent to `ImGui::NewFrame`.
     */
    void beginImguiFrame();

    /**
     * @brief End an ImGui frame.
     *
     * Torch's equivalent to `ImGui::EndFrame`.
     */
    void endImguiFrame();

    /**
     * @brief ImGui render plugin factory.
     *
     * Add this function to `TorchStackCreateInfo::plugins` to add the ImGui
     * plugin to Torch's render pipeline.
     */
    auto buildImguiRenderPlugin(Window& window) -> PluginBuilder;

    class ImguiRenderPlugin : public RenderPlugin
    {
    public:
        ImguiRenderPlugin(const ImguiRenderPlugin&) = delete;
        ImguiRenderPlugin(ImguiRenderPlugin&&) noexcept = delete;
        ImguiRenderPlugin& operator=(const ImguiRenderPlugin&) = delete;
        ImguiRenderPlugin& operator=(ImguiRenderPlugin&&) noexcept = delete;

        explicit ImguiRenderPlugin(Window& window);
        ~ImguiRenderPlugin() noexcept override;

        void defineRenderStages(RenderGraph& graph) override;
        void defineResources(ResourceConfig& resources) override;

        auto createGlobalResources(RenderPipelineContext& ctx)
            -> u_ptr<GlobalResources> override;

    private:
        class ImguiDrawResources : public GlobalResources
        {
        public:
            void registerResources(ResourceStorage& resources) override;

            void hostUpdate(RenderPipelineContext& ctx) override;
            void createTasks(GlobalUpdateTaskQueue& queue) override;

        private:
            static void dispatchImguiCommands(vk::CommandBuffer cmdBuf, GlobalUpdateContext& ctx);
        };

        GLFWwindow* window;
    };
} // namespace trc::imgui
