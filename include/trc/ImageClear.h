#pragma once

#include "trc/core/RenderPipelineTasks.h"
#include "trc/core/RenderPlugin.h"

namespace trc
{
    /**
     * @brief Specifies how to clear an image.
     */
    struct ImageClearInfo
    {
        // The image to clear.
        vk::Image image;

        // A subresource range that shall be cleared on the image.
        vk::ImageSubresourceRange range;

        // The color to which to clear the image.
        vk::ClearColorValue clearColor;
    };

    /**
     * @brief A task that clears an image to a constant color.
     *
     * Submits a call to `vkCmdClearColorImage`.
     *
     * Note: The cleared image *must* have `VK_IMAGE_USAGE_TRANSFER_DST_BIT`
     * set. "Image must have been created with VK_IMAGE_USAGE_TRANSFER_DST_BIT
     * usage flag"
     * (https://registry.khronos.org/vulkan/specs/latest/html/vkspec.html#VUID-vkCmdClearColorImage-image-00002)
     */
    class ImageClearTask : public DeviceTask
    {
    public:
        explicit ImageClearTask(const ImageClearInfo& info);
        void record(vk::CommandBuffer cmdBuf, DeviceExecutionContext& ctx) override;

    private:
        const ImageClearInfo info;
    };

    /**
     * @brief Build the `RenderTargetImageClearPlugin` render pipeline plugin.
     *
     * See `RenderTargetImageClearPlugin` for more information.
     *
     * @param clearColor The color to which the render target image will be
     *                   cleared.
     */
    auto buildRenderTargetImageClearPlugin(vk::ClearColorValue clearColor) -> PluginBuilder;

    /**
     * @brief A render plugin that clears the render target image.
     *
     * Submits a `vkCmdClearColorImage` call that depends on the
     * `trc::stages::renderTargetImageInit` stage.
     *
     * Note: The render target image *must* have `VK_IMAGE_USAGE_TRANSFER_DST_BIT`
     * set. If you render to a swapchain image, specify this bit during
     * swapchain/window creation.
     */
    class RenderTargetImageClearPlugin : public RenderPlugin
    {
    public:
        explicit RenderTargetImageClearPlugin(vk::ClearColorValue clearColor);

        void defineRenderStages(RenderGraph& graph) override;
        void defineResources(ResourceConfig&) override {}

        auto createGlobalResources(RenderPipelineContext& ctx)
            -> u_ptr<GlobalResources> override;

    private:
        struct Config : GlobalResources
        {
            Config(const vk::ClearColorValue& col) : clearColor(col) {}

            void registerResources(ResourceStorage&) override {}
            void hostUpdate(RenderPipelineContext&) override {}
            void createTasks(GlobalUpdateTaskQueue& queue) override;

            vk::ClearColorValue clearColor;
        };

        static inline RenderStage clearStage = RenderStage::make("RenderTargetClear");

        vk::ClearColorValue clearColor;
    };
} // namespace trc
