#include "trc/SwapchainPlugin.h"

#include "trc/TorchRenderStages.h"
#include "trc/core/RenderGraph.h"
#include "trc/core/Task.h"



namespace trc
{

void SwapchainPlugin::registerRenderStages(RenderGraph& graph)
{
    graph.insert(renderTargetImageInitStage);
    graph.insert(renderTargetImageFinalizeStage);
}

auto SwapchainPlugin::createDrawConfig(const Device& /*device*/, Viewport renderTarget)
    -> u_ptr<DrawConfig>
{
    return std::make_unique<SwapchainPluginDrawConfig>(renderTarget.image);
}

void SwapchainPlugin::SwapchainPluginDrawConfig::createTasks(
    SceneBase& /*scene*/,
    TaskQueue& taskQueue)
{
    // Create accesses to swapchain image in pre- and post stages
    taskQueue.spawnTask(renderTargetImageInitStage,
        makeTask([image=this->image](vk::CommandBuffer, TaskEnvironment& env) {
            env.produce(ImageAccess{
                image,
                vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1),
                vk::PipelineStageFlagBits2::eTopOfPipe,
                vk::AccessFlagBits2::eMemoryWrite | vk::AccessFlagBits2::eMemoryRead,
                vk::ImageLayout::eUndefined
            });
        })
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
    taskQueue.spawnTask(renderTargetImageFinalizeStage,
        makeTask([image=this->image](vk::CommandBuffer, TaskEnvironment& env) {
            env.consume(ImageAccess{
                image,
                vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1),
                vk::PipelineStageFlagBits2::eBottomOfPipe,
                vk::AccessFlagBits2::eNone,
                vk::ImageLayout::ePresentSrcKHR
            });
        })
    );
}

} // namespace trc
