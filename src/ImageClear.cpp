#include "trc/ImageClear.h"

#include "trc/TorchRenderStages.h"
#include "trc/core/RenderGraph.h"



namespace trc
{

ImageClearTask::ImageClearTask(const ImageClearInfo& info)
    : info(info)
{
}

void ImageClearTask::record(vk::CommandBuffer cmdBuf, DeviceExecutionContext& ctx)
{
    ctx.deps().consume(ImageAccess{
        info.image, info.range,
        vk::PipelineStageFlagBits2::eTransfer,
        vk::AccessFlagBits2::eTransferWrite,
        vk::ImageLayout::eTransferDstOptimal
    });

    cmdBuf.clearColorImage(
        info.image,
        vk::ImageLayout::eTransferDstOptimal,
        info.clearColor,
        info.range
    );

    ctx.deps().produce(ImageAccess{
        info.image, info.range,
        vk::PipelineStageFlagBits2::eTransfer,
        vk::AccessFlagBits2::eTransferWrite,
        vk::ImageLayout::eTransferDstOptimal
    });
}



auto buildRenderTargetImageClearPlugin(vk::ClearColorValue clearColor) -> PluginBuilder
{
    return [=](auto&&){
        return std::make_unique<trc::RenderTargetImageClearPlugin>(clearColor);
    };
}



RenderTargetImageClearPlugin::RenderTargetImageClearPlugin(vk::ClearColorValue clearColor)
    : clearColor(clearColor)
{
}

void RenderTargetImageClearPlugin::defineRenderStages(RenderGraph& graph)
{
    graph.createOrdering(stages::renderTargetImageInit, clearStage);
    graph.createOrdering(clearStage, stages::gBuffer);
    graph.createOrdering(clearStage, stages::renderTargetImageFinalize);
}

auto RenderTargetImageClearPlugin::createGlobalResources(RenderPipelineContext&)
    -> u_ptr<GlobalResources>
{
    return std::make_unique<Config>(clearColor);
}

void RenderTargetImageClearPlugin::Config::createTasks(GlobalUpdateTaskQueue& queue)
{
    queue.spawnTask(
        clearStage,
        [col=clearColor](vk::CommandBuffer cmdBuf, GlobalUpdateContext& ctx)
        {
            ImageClearInfo info{
                ctx.renderTargetImage().image,
                vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1),
                col,
            };

            ImageClearTask{ info }.record(cmdBuf, ctx);
        }
    );
}

} // namespace trc
