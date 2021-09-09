#pragma once

#include <vkb/QueueManager.h>

namespace trc::util
{
    /**
     * Try to reserve a queue. Order:
     *  1. Reserve primary queue
     *  2. Reserve any queue
     *  3. Don't reserve, just return any queue
     */
    inline auto tryReserve(vkb::QueueManager& qm, vkb::QueueType type)
        -> std::pair<vkb::ExclusiveQueue, vkb::QueueFamilyIndex>
    {
        if (qm.getPrimaryQueueCount(type) > 1)
        {
            return { qm.reservePrimaryQueue(type), qm.getPrimaryQueueFamily(type) };
        }
        else if (qm.getAnyQueueCount(type) > 1)
        {
            auto [queue, family] = qm.getAnyQueue(type);
            return { qm.reserveQueue(queue), family };
        }
        else {
            return qm.getAnyQueue(type);
        }
    };
} // namespace trc::util
