#include "trc/ui/torch/GuiIntegration.h"

#include "trc/TorchRenderStages.h"
#include "trc/core/RenderGraph.h"



namespace trc
{

auto buildGuiRenderPlugin(s_ptr<ui::Window> window) -> TorchPipelinePluginBuilder
{
    return [window](auto&&) {
        return [window](PluginBuildContext&) -> u_ptr<RenderPlugin> {
            return std::make_unique<GuiRenderPlugin>(window);
        };
    };
}



GuiRenderPlugin::GuiRenderPlugin(s_ptr<ui::Window> window)
    :
    window(std::move(window))
{
}

void GuiRenderPlugin::defineRenderStages(RenderGraph& graph)
{
    graph.createOrdering(stages::post, guiRenderStage);
    graph.createOrdering(guiRenderStage, stages::renderTargetImageFinalize);
}

void GuiRenderPlugin::defineResources(ResourceConfig& /*config*/)
{
    // Nothing to do.
}

auto GuiRenderPlugin::createViewportResources(ViewportContext& ctx)
    -> u_ptr<ViewportResources>
{
    return std::make_unique<GuiViewportConfig>(window, ctx.device(), ctx.renderImage());
}



GuiRenderPlugin::GuiViewportConfig::GuiViewportConfig(
    const s_ptr<ui::Window>& window,
    const Device& device,
    const RenderImage& image)
    :
    dstImage(image),
    window(window),
    collector(device, image.format)
{
}

void GuiRenderPlugin::GuiViewportConfig::registerResources(ResourceStorage& /*resources*/)
{
    // Nothing to do.
}

void GuiRenderPlugin::GuiViewportConfig::hostUpdate(ViewportContext& /*ctx*/)
{
    drawList = window->draw();

    // Sort draw list by type of drawable
    std::ranges::sort(
        drawList,
        [](const auto& a, const auto& b) {
            return a.type.index() < b.type.index();
        }
    );
}

void GuiRenderPlugin::GuiViewportConfig::createTasks(
    ViewportDrawTaskQueue& queue,
    ViewportContext&)
{
    if (drawList.empty()) return;

    queue.spawnTask(guiRenderStage,
        [this](vk::CommandBuffer cmdBuf, ViewportDrawContext& ctx)
        {
            ctx.deps().consume(ImageAccess{
                dstImage.image,
                vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1),
                vk::PipelineStageFlagBits2::eColorAttachmentOutput,
                vk::AccessFlagBits2::eColorAttachmentWrite,
                vk::ImageLayout::eColorAttachmentOptimal
            });

            vk::RenderingAttachmentInfo colorAttachment{
                ctx.renderImage().imageView,
                vk::ImageLayout::eColorAttachmentOptimal,
                vk::ResolveModeFlagBits::eNone, VK_NULL_HANDLE, vk::ImageLayout::eUndefined, // resolve
                vk::AttachmentLoadOp::eLoad, vk::AttachmentStoreOp::eStore,
                vk::ClearColorValue(std::array<float, 4>{{ 0.0f, 0.0f, 0.0f, 0.0f }}),
            };
            cmdBuf.beginRendering(vk::RenderingInfo{
                vk::RenderingFlags{},
                { 0, { dstImage.size.x, dstImage.size.y } },
                1,                // layer count
                0x00,             // view mask
                colorAttachment,
                nullptr, nullptr  // depth and stencil attachments
            });

            // Record all element commands
            collector.beginFrame();
            for (const auto& info : drawList) {
                collector.drawElement(info);
            }
            collector.endFrame(cmdBuf, dstImage.size);
            drawList.clear();

            cmdBuf.endRendering();

            ctx.deps().produce(ImageAccess{
                dstImage.image,
                vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1),
                vk::PipelineStageFlagBits2::eColorAttachmentOutput,
                vk::AccessFlagBits2::eColorAttachmentWrite,
                vk::ImageLayout::eColorAttachmentOptimal
            });
        }
    );
}

} // namespace trc
