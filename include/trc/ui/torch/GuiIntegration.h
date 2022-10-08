#pragma once

#include <mutex>
#include <future>

#include <trc_util/Timer.h>
#include "trc/base/Buffer.h"
#include "trc/base/FrameSpecificObject.h"
#include "trc/base/Image.h"
#include "trc/base/event/Event.h"

#include "trc/Framebuffer.h"
#include "trc/core/Pipeline.h"
#include "trc/core/RenderPass.h"
#include "trc/core/RenderStage.h"
#include "trc/ui/Window.h"
#include "trc/ui/torch/GuiRenderer.h"

namespace trc
{
    class RenderLayout;

    inline RenderStage guiRenderStage{};

    class TorchWindowBackend : public ui::WindowBackend
    {
    public:
        explicit TorchWindowBackend(const Swapchain& swapchain)
            : swapchain(swapchain)
        {}

        auto getSize() -> vec2 override
        {
            return { swapchain.getImageExtent().width, swapchain.getImageExtent().height };
        }

    private:
        const Swapchain& swapchain;
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
                           const Swapchain& swapchain,
                           ui::Window& window,
                           GuiRenderer& renderer);
        ~GuiIntegrationPass();

        void begin(vk::CommandBuffer, vk::SubpassContents, FrameRenderState&) override;
        void end(vk::CommandBuffer) override;

    private:
        const Device& device;
        const Swapchain& swapchain;

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
    auto initGui(Device& device, const Swapchain& swapchain) -> GuiStack;

    /**
     * @brief Insert gui renderpass into a render layout
     */
    void integrateGui(GuiStack& stack, RenderLayout& layout);
} // namespace trc
