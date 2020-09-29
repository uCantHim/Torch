#pragma once

/**
 * Convenience header that includes all common event related headers
 */

#include "EventHandler.h"

#include "InputEvents.h"
#include "WindowEvents.h"

#include "Keys.h"
#include "InputState.h"

namespace vkb
{
    template<typename EventType>
    inline auto on(std::function<void(const EventType&)> callback) -> UniqueListenerId<EventType>
    {
        return EventHandler<EventType>::addListener(std::move(callback));
    }
}
