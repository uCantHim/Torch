#pragma once

#include "input/InputStructs.h"

class EventTarget
{
public:
    virtual ~EventTarget() noexcept = default;

    virtual void notify(const UserInput& input) = 0;
    virtual void notify(const Scroll& scroll) = 0;
    virtual void notify(const CursorMovement& cursorMove) = 0;
};
