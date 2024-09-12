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
