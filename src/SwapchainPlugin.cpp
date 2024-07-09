#include "trc/SwapchainPlugin.h"

#include "trc/TorchRenderStages.h"
#include "trc/base/Swapchain.h"
#include "trc/core/DataFlow.h"
#include "trc/core/RenderGraph.h"



namespace trc
{

SwapchainPlugin::SwapchainPlugin(const Swapchain& swapchain)
    :
    swapchain(swapchain)
{
}

void SwapchainPlugin::defineRenderStages(RenderGraph& graph)
{
    graph.insert(renderTargetImageInitStage);
    graph.insert(renderTargetImageFinalizeStage);
}

auto SwapchainPlugin::createGlobalResources(RenderPipelineContext&)
    -> u_ptr<GlobalResources>
{
    return std::make_unique<Instance>(swapchain);
}

void SwapchainPlugin::Instance::createTasks(GlobalUpdateTaskQueue& queue)
{
    // Create accesses to swapchain image in pre- and post stages
    queue.spawnTask(renderTargetImageInitStage,
        [this](vk::CommandBuffer, GlobalUpdateContext& ctx) {
            ctx.deps().produce(ImageAccess{
                swapchain.getImage(swapchain.getCurrentFrame()),
                vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1),
                vk::PipelineStageFlagBits2::eTopOfPipe,
                vk::AccessFlagBits2::eMemoryWrite | vk::AccessFlagBits2::eMemoryRead,
                vk::ImageLayout::eUndefined
            });
        }
    );

    /**
     * Spec states:
     *
     * When transitioning the image to VK_IMAGE_LAYOUT_SHARED_PRESENT_KHR or
     * VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, there is no need to delay subsequent
     * processing, or perform any visibility operations (as vkQueuePresentKHR
     * performs automatic visibility operations). To achieve this, the
     * dstAccessMask member of the VkImageMemoryBarrier should be set to 0, and
     * the dstStageMask parameter should be set to
     * VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT.
     */
    queue.spawnTask(renderTargetImageFinalizeStage,
        [this](vk::CommandBuffer, GlobalUpdateContext& ctx) {
            ctx.deps().consume(ImageAccess{
                swapchain.getImage(swapchain.getCurrentFrame()),
                vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1),
                vk::PipelineStageFlagBits2::eBottomOfPipe,
                vk::AccessFlagBits2::eNone,
                vk::ImageLayout::ePresentSrcKHR
            });
        }
    );
}

} // namespace trc
