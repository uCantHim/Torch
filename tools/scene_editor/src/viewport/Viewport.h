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

    virtual void notify(const UserInput& input) override = 0;
    virtual void notify(const Scroll& scroll) override = 0;
    virtual void notify(const CursorMovement& cursorMove) override = 0;
};
