#pragma once

#include <mutex>
#include <future>

#include <vkb/Buffer.h>
#include <vkb/Image.h>
#include <vkb/FrameSpecificObject.h>
#include <vkb/event/Event.h>

#include "RenderStage.h"
#include "RenderPass.h"
#include "Pipeline.h"
#include "Framebuffer.h"
#include "ui/Window.h"
#include "ui/torch/GuiRenderer.h"
#include "util/Timer.h"

namespace trc
{
    class RenderLayout;

    inline RenderStage guiRenderStage{};

    class TorchWindowBackend : public ui::WindowBackend
    {
    public:
        explicit TorchWindowBackend(const vkb::Swapchain& swapchain)
            : swapchain(swapchain)
        {}

        auto getSize() -> vec2 override
        {
            return { swapchain.getImageExtent().width, swapchain.getImageExtent().height };
        }

    private:
        const vkb::Swapchain& swapchain;
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
        GuiIntegrationPass(const vkb::Device& device,
                           const vkb::Swapchain& swapchain,
                           ui::Window& window,
                           GuiRenderer& renderer);
        ~GuiIntegrationPass();

        void begin(vk::CommandBuffer, vk::SubpassContents) override;
        void end(vk::CommandBuffer) override;

    private:
        const vkb::Device& device;
        const vkb::Swapchain& swapchain;

        std::mutex renderLock;
        std::thread renderThread;
        bool stopRenderThread{ false };

        void createDescriptorSets();
        void writeDescriptorSets(vk::ImageView srcImage);

        GuiRenderTarget renderTarget;

        vk::UniqueDescriptorPool blendDescPool;
        vk::UniqueDescriptorSetLayout blendDescLayout;
        vkb::FrameSpecific<vk::UniqueDescriptorSet> blendDescSets;
        std::vector<vk::UniqueImageView> swapchainImageViews;
        PipelineLayout imageBlendPipelineLayout;
        Pipeline imageBlendPipeline;

        vkb::UniqueListenerId<vkb::SwapchainRecreateEvent> swapchainRecreateListener;
    };

    struct GuiStack
    {
        u_ptr<ui::Window> window;

        u_ptr<GuiRenderer> renderer;
        u_ptr<GuiIntegrationPass> renderPass;

        vkb::UniqueListenerId<vkb::MouseClickEvent> mouseClickListener;

        vkb::UniqueListenerId<vkb::KeyPressEvent>   keyPressListener;
        vkb::UniqueListenerId<vkb::KeyRepeatEvent>  keyRepeatListener;
        vkb::UniqueListenerId<vkb::KeyReleaseEvent> keyReleaseListener;
        vkb::UniqueListenerId<vkb::CharInputEvent>  charInputListener;
    };

    /**
     * @brief Initialize the GUI implementation
     */
    auto initGui(vkb::Device& device, const vkb::Swapchain& swapchain) -> GuiStack;

    /**
     * @brief Insert gui renderpass into a render layout
     */
    void integrateGui(GuiStack& stack, RenderLayout& layout);
} // namespace trc
