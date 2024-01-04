#pragma once

#include <mutex>
#include <thread>

#include <trc_util/Timer.h>

#include "trc/base/FrameSpecificObject.h"
#include "trc/core/Pipeline.h"
#include "trc/core/RenderPass.h"
#include "trc/core/RenderStage.h"
#include "trc/core/RenderTarget.h"
#include "trc/ui/Window.h"
#include "trc/ui/torch/GuiRenderer.h"

namespace trc
{
    class RenderGraph;

    inline RenderStage guiRenderStage = makeRenderStage();

    class TorchWindowBackend : public ui::WindowBackend
    {
    public:
        explicit TorchWindowBackend(const RenderTarget& renderTarget)
            : renderTarget(renderTarget)
        {}

        auto getSize() -> vec2 override {
            return renderTarget.getSize();
        }

    private:
        const RenderTarget& renderTarget;
    };

    /**
     * Render pass that integrates the gui into Torch's render pipeline.
     *
     * Actually, the GuiRenderer class contains the vkRenderPass instance.
     * This class is merely the component that ties the GUI rendering to
     * Torch's render pipeline.
     */
    class GuiIntegrationPass : public RenderPass
    {
    public:
        GuiIntegrationPass(const Device& device,
                           RenderTarget renderTarget,
                           ui::Window& window);
        ~GuiIntegrationPass();

        void begin(vk::CommandBuffer, vk::SubpassContents, FrameRenderState&) override;
        void end(vk::CommandBuffer) override;

        void setRenderTarget(RenderTarget renderTarget);

    private:
        const Device& device;
        RenderTarget target;

        std::mutex renderLock;
        std::thread renderThread;
        bool stopRenderThread{ false };

        void createDescriptorSets();
        void writeDescriptorSets(vk::ImageView srcImage);

        GuiRenderer renderer;
        GuiRenderTarget guiImage;

        vk::UniqueDescriptorPool blendDescPool;
        vk::UniqueDescriptorSetLayout blendDescLayout;
        FrameSpecific<vk::UniqueDescriptorSet> blendDescSets;
        PipelineLayout imageBlendPipelineLayout;
        Pipeline imageBlendPipeline;
    };

    /**
     * @brief Initialize the GUI implementation
     *
     * Creates a key map for the `ui::Window` that maps Torch's key definitions
     * in `trc/base/event/Keys.h` to key events for the window.
     */
    auto makeGui(const Device& device, const RenderTarget& renderTarget)
        -> std::pair<u_ptr<ui::Window>, u_ptr<GuiIntegrationPass>>;

    /**
     * @brief Insert gui renderpass into a render layout
     *
     * Inserts the render pass into the graph at the `trc::guiRenderStage` stage.
     */
    void integrateGui(GuiIntegrationPass& renderPass, RenderGraph& graph);
} // namespace trc
