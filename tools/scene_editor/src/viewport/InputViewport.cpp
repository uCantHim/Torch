#include "viewport/InputViewport.h"



auto InputViewport::notify(const UserInput& input) -> NotifyResult
{
    return inputHandler.notify(input);
}

auto InputViewport::notify(const Scroll& scroll) -> NotifyResult
{
    return inputHandler.notify(scroll);
}

auto InputViewport::notify(const CursorMovement& cursorMove) -> NotifyResult
{
    return inputHandler.notify(cursorMove);
}

auto InputViewport::getInputHandler() -> InputFrame&
{
    return inputHandler.getRootFrame();
}
