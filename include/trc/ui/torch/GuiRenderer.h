#pragma once

#include "trc/base/Buffer.h"
#include "trc/base/Image.h"

#include "trc/Framebuffer.h"
#include "trc/ui/Window.h"
#include "trc/ui/torch/DrawImplementations.h"

namespace trc
{
    class GuiRenderTarget
    {
    public:
        GuiRenderTarget(const Device& device, vk::RenderPass renderPass, uvec2 size);

        auto getSize() const -> uvec2;

        auto getImage() -> Image&;
        auto getFramebuffer() const -> const Framebuffer&;

    private:
        Image image;
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
        GuiRenderer(Device& device);

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

        void notifyNewFont(ui32 fontIndex, GlyphCache& cache);

    private:
        const Device& device;

        ExclusiveQueue renderQueue;
        vk::UniqueFence renderFinishedFence;
        vk::UniqueCommandPool cmdPool;
        vk::UniqueCommandBuffer cmdBuf;

        vk::UniqueRenderPass renderPass;
        const vk::ClearValue clearValue;

        ui_impl::DrawCollector collector;
    };
} // namespace trc
