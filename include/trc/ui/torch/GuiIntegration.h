#pragma once

#include <mutex>
#include <future>

#include <vkb/Buffer.h>
#include <vkb/Image.h>
#include <vkb/util/Timer.h>

#include "RenderStage.h"
#include "RenderPass.h"
#include "Pipeline.h"
#include "ui/Window.h"
#include "ui/torch/DrawImplementations.h"

namespace trc
{
    class Renderer;

    /**
     * @brief Initialize the GUI implementation
     */
    auto initGui(Renderer& renderer) -> u_ptr<ui::Window>;

    /**
     * Access to static render stage
     */
    auto getGuiRenderStage() -> RenderStageType::ID;

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
     * Render a GUI root to an image
     */
    class GuiRenderer
    {
    public:
        GuiRenderer(vkb::Device& device, ui::Window& window);

        void render();

        auto getRenderPass() const -> vk::RenderPass;
        auto getOutputImage() const -> vk::Image;
        auto getOutputImageView() const -> vk::ImageView;

    private:
        const vkb::Device& device;
        ui::Window* window;

        vk::Queue renderQueue;
        vk::UniqueFence renderFinishedFence;
        vk::UniqueCommandPool cmdPool;
        vk::UniqueCommandBuffer cmdBuf;

        vk::UniqueRenderPass renderPass;
        vkb::Image outputImage;
        vk::UniqueImageView outputImageView;
        vk::UniqueFramebuffer framebuffer;

        ui_impl::DrawCollector collector;
    };

    /**
     * Render pass that integrates the gui into Torch's render pipeline.
     *
     * Actually, the GuiRenderer class contains the vkRenderPass instance.
     * This class is merely the component that ties the GUI rendering to
     * Torch's render pipeline.
     */
    class GuiRenderPass : public RenderPass
    {
    public:
        GuiRenderPass(vkb::Device& device,
                      const vkb::Swapchain& swapchain,
                      ui::Window& window);
        ~GuiRenderPass();

        void begin(vk::CommandBuffer, vk::SubpassContents) override;
        void end(vk::CommandBuffer) override;

    private:
        const vkb::Device& device;

        GuiRenderer renderer;
        std::mutex renderLock;
        std::thread renderThread;
        bool stopRenderThread{ false };

        void createDescriptorSets(const vkb::Device& device, const vkb::Swapchain& swapchain);
        vk::UniqueDescriptorPool blendDescPool;
        vk::UniqueDescriptorSetLayout blendDescLayout;
        vkb::FrameSpecificObject<vk::UniqueDescriptorSet> blendDescSets;
        std::vector<vk::UniqueImageView> swapchainImageViews;
        Pipeline::ID imageBlendPipeline;
    };
} // namespace trc
