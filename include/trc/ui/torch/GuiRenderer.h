#pragma once

#include <vkb/Buffer.h>
#include <vkb/Image.h>

#include "Framebuffer.h"
#include "ui/Window.h"
#include "ui/torch/DrawImplementations.h"

namespace trc
{
    class GuiRenderTarget
    {
    public:
        GuiRenderTarget(const vkb::Device& device, vk::RenderPass renderPass, uvec2 size);

        auto getSize() const -> uvec2;

        auto getImage() -> vkb::Image&;
        auto getFramebuffer() const -> const Framebuffer&;

    private:
        vkb::Image image;
        Framebuffer framebuffer;
    };

    /**
     * @brief Renders a GUI root to an image
     */
    class GuiRenderer
    {
    public:
        /**
         * TODO: Don't let the GuiRenderer decide which queue to use, pass
         *       one as a parameter
         */
        GuiRenderer(vkb::Device& device);

        /**
         * @brief Draw a gui window's contents to the Renderer's image
         *
         * When this function returns, all device commands will have
         * completed execution. This synchronous execution may take quite
         * a while.
         *
         * Can be called asynchronously on the host.
         *
         * TODO: Maybe return a future here that is satifsfied when device
         *       execution completes?
         */
        void render(ui::Window& window, GuiRenderTarget& target);

        auto getRenderPass() const -> vk::RenderPass;

    private:
        const vkb::Device& device;

        vkb::ExclusiveQueue renderQueue;
        vk::UniqueFence renderFinishedFence;
        vk::UniqueCommandPool cmdPool;
        vk::UniqueCommandBuffer cmdBuf;

        vk::UniqueRenderPass renderPass;
        const vk::ClearValue clearValue;

        ui_impl::DrawCollector collector;
    };
} // namespace trc
