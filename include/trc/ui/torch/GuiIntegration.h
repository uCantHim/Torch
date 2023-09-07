#pragma once

#include <future>
#include <mutex>
#include <thread>

#include <trc_util/Timer.h>

#include "trc/Framebuffer.h"
#include "trc/base/Buffer.h"
#include "trc/base/FrameSpecificObject.h"
#include "trc/base/Image.h"
#include "trc/base/event/Event.h"
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
                           const RenderTarget& renderTarget,
                           ui::Window& window,
                           GuiRenderer& renderer);
        ~GuiIntegrationPass();

        void begin(vk::CommandBuffer, vk::SubpassContents, FrameRenderState&) override;
        void end(vk::CommandBuffer) override;

    private:
        const Device& device;
        const RenderTarget& target;

        std::mutex renderLock;
        std::thread renderThread;
        bool stopRenderThread{ false };

        void createDescriptorSets();
        void writeDescriptorSets(vk::ImageView srcImage);

        GuiRenderTarget renderTarget;

        vk::UniqueDescriptorPool blendDescPool;
        vk::UniqueDescriptorSetLayout blendDescLayout;
        FrameSpecific<vk::UniqueDescriptorSet> blendDescSets;
        PipelineLayout imageBlendPipelineLayout;
        Pipeline imageBlendPipeline;

        UniqueListenerId<SwapchainRecreateEvent> swapchainRecreateListener;
    };

    struct GuiStack
    {
        u_ptr<ui::Window> window;

        u_ptr<GuiRenderer> renderer;
        u_ptr<GuiIntegrationPass> renderPass;

        UniqueListenerId<MouseClickEvent> mouseClickListener;

        UniqueListenerId<KeyPressEvent>   keyPressListener;
        UniqueListenerId<KeyRepeatEvent>  keyRepeatListener;
        UniqueListenerId<KeyReleaseEvent> keyReleaseListener;
        UniqueListenerId<CharInputEvent>  charInputListener;
    };

    /**
     * @brief Initialize the GUI implementation
     */
    auto initGui(const Device& device, const RenderTarget& renderTarget) -> GuiStack;

    /**
     * @brief Insert gui renderpass into a render layout
     */
    void integrateGui(GuiStack& stack, RenderGraph& graph);
} // namespace trc
