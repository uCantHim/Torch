#pragma once

#include "input/InputState.h"
#include "viewport/Viewport.h"

/**
 * @brief A viewport that forwards its inputs to a concrete input handler.
 */
class InputViewport : public Viewport
{
public:
    void notify(const UserInput& input) final;
    void notify(const Scroll& scroll) final;
    void notify(const CursorMovement& cursorMove) final;

    auto getInputHandler() -> InputFrame&;

private:
    InputStateMachine inputHandler;
};
