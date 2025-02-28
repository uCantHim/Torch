#pragma once

#include "input/InputStructs.h"

class EventTarget
{
public:
    enum class NotifyResult
    {
        // The event has been consumed and should not be further processed
        // by the caller.
        eConsumed,

        // The event has not been consumed and may be processed by the caller.
        eRejected,
    };

    virtual ~EventTarget() noexcept = default;

    virtual auto notify(const UserInput& input) -> NotifyResult = 0;
    virtual auto notify(const Scroll& scroll) -> NotifyResult = 0;
    virtual auto notify(const CursorMovement& cursorMove) -> NotifyResult = 0;
};
