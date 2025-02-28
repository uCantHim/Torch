#pragma once

#include <trc/core/Frame.h>

#include "input/EventTarget.h"

struct ViewportArea
{
    ivec2 pos;
    uvec2 size;
};

class Viewport : public EventTarget
{
public:
    virtual ~Viewport() noexcept = default;

    virtual void draw(trc::Frame& frame) = 0;

    virtual void resize(const ViewportArea& newArea) = 0;
    virtual auto getSize() -> ViewportArea = 0;

    virtual auto notify(const UserInput& input) -> NotifyResult override = 0;
    virtual auto notify(const Scroll& scroll) -> NotifyResult override = 0;
    virtual auto notify(const CursorMovement& cursorMove) -> NotifyResult override = 0;
};
