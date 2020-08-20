#pragma once

#include <vkb/VulkanBase.h>

#include "Boilerplate.h"
#include "RenderPass.h"
#include "base/SceneBase.h"
#include "utils/Camera.h"

namespace trc
{
    class CommandCollector
    {
    public:
        CommandCollector();

        /**
         * Collect commands of a scene and write them to a primary command buffer
         */
        auto recordScene(SceneBase& scene, RenderStage::ID renderStage)
            -> vk::CommandBuffer;

    private:
        vk::UniqueCommandPool pool;
        vkb::FrameSpecificObject<vk::UniqueCommandBuffer> commandBuffers;
    };
} // namespace trc
