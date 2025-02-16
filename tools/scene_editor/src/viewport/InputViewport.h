#pragma once

#include "input/InputHandler.h"
#include "viewport/Viewport.h"

/**
 * @brief A viewport that forwards its inputs to a concrete input handler.
 */
class InputViewport : public Viewport
{
public:
    auto notify(const UserInput& input) -> NotifyResult final;
    auto notify(const Scroll& scroll) -> NotifyResult final;
    auto notify(const CursorMovement& cursorMove) -> NotifyResult final;

    auto getInputHandler() -> InputFrame&;

private:
    InputHandler inputHandler;
};
