#pragma once

#include <vector>

#include <vkb/basics/Device.h>

#include "Types.h"
#include "Instance.h"
#include "RenderPass.h"
#include "DrawConfiguration.h"

namespace trc
{
    class CommandCollector
    {
    public:
        CommandCollector(const Window& window, vkb::QueueFamilyIndex renderQueueFamily);

        /**
         * Collect commands of a scene and write them to a primary command buffer
         */
        auto recordScene(const DrawConfig& draw, const RenderGraph& graph)
            -> std::vector<vk::CommandBuffer>;

    private:
        void recordStage(vk::CommandBuffer cmdBuf,
                         const DrawConfig& draw,
                         RenderStageType::ID stage,
                         const std::vector<RenderPass*>& passes);

        const vkb::Device& device;
        const vkb::Swapchain& swapchain;

        vk::UniqueCommandPool pool;
        std::vector<vkb::FrameSpecific<vk::UniqueCommandBuffer>> commandBuffers;
    };
} // namespace trc
