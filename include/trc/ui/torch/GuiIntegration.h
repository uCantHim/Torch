#pragma once

#include <trc_util/Timer.h>

#include "trc/Framebuffer.h"
#include "trc/Torch.h"
#include "trc/core/Pipeline.h"
#include "trc/core/RenderPlugin.h"
#include "trc/core/RenderStage.h"
#include "trc/core/RenderTarget.h"
#include "trc/core/RenderTarget.h"
#include "trc/ui/Window.h"
#include "trc/ui/torch/DrawImplementations.h"

namespace trc
{
    class RenderGraph;

    inline const RenderStage guiRenderStage = makeRenderStage();

    auto buildGuiRenderPlugin(s_ptr<ui::Window> window) -> TorchPipelinePluginBuilder;

    /**
     * @brief Render plugin implementation for Torch's gui framework.
     *
     * Naturally, everything related to Torch's UI is highly experimental and
     * more of a proof-of-concept right now.
     */
    class GuiRenderPlugin : public RenderPlugin
    {
    public:
        explicit GuiRenderPlugin(s_ptr<ui::Window> window);

        void defineRenderStages(RenderGraph& graph) override;
        void defineResources(ResourceConfig& config) override;

        auto createViewportResources(ViewportContext& ctx)
            -> u_ptr<ViewportResources> override;

    private:
        class GuiViewportConfig : public ViewportResources
        {
        public:
            GuiViewportConfig(const s_ptr<ui::Window>& window,
                              const Device& device,
                              const RenderImage& dstImage);

            void registerResources(ResourceStorage& resources) override;
            void hostUpdate(ViewportContext& ctx) override;
            void createTasks(ViewportDrawTaskQueue& queue, ViewportContext& ctx) override;

        private:
            const RenderImage dstImage;

            s_ptr<ui::Window> window;
            ui_impl::DrawCollector collector;
            ui::DrawList drawList;
        };

        s_ptr<ui::Window> window;
    };
} // namespace trc
