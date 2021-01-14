#pragma once

#include <vector>

#include <vkb/basics/Instance.h>
#include <vkb/basics/Device.h>
#include <vkb/ExclusiveQueue.h>

#include "RenderStage.h"

/**
 * This file provides access to internally used resources
 */
namespace trc
{
    /**
     * @brief Access to global render stage types
     */
    struct RenderStageTypes
    {
        static auto getDeferred() -> RenderStageType::ID;
        static auto getShadow() -> RenderStageType::ID;
    };

    /**
     * @brief Access to global queue resources
     */
    struct Queues
    {
        static void init(vkb::QueueManager& queueManager);

        static auto getMainRender() -> vkb::ExclusiveQueue&;
        static auto getMainRenderFamily() -> vkb::QueueFamilyIndex;
        static auto getMainPresent() -> vkb::ExclusiveQueue&;
        static auto getMainPresentFamily() -> vkb::QueueFamilyIndex;
        static auto getTransfer() -> vkb::ExclusiveQueue&;
    };
} // namespace trc
