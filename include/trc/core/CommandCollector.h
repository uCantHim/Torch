#pragma once

#include <vkb/VulkanBase.h>

#include "Types.h"
#include "Instance.h"
#include "RenderPass.h"
#include "DrawConfiguration.h"

namespace trc
{
    class CommandCollector
    {
    public:
        explicit CommandCollector(const Instance& instance, const Window& window);

        /**
         * Collect commands of a scene and write them to a primary command buffer
         */
        auto recordScene(const DrawConfig& draw,
                         RenderStageType::ID stageType,
                         const std::vector<RenderPass*>& passes)
            -> vk::CommandBuffer;

    private:
        vk::UniqueCommandPool pool;
        vkb::FrameSpecificObject<vk::UniqueCommandBuffer> commandBuffers;
    };
} // namespace trc
