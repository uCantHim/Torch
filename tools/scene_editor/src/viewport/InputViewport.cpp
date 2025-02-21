#include "viewport/InputViewport.h"



void InputViewport::notify(const UserInput& input)
{
    inputHandler.notify(input);
}

void InputViewport::notify(const Scroll& scroll)
{
    inputHandler.notify(scroll);
}

void InputViewport::notify(const CursorMovement& cursorMove)
{
    inputHandler.notify(cursorMove);
}

auto InputViewport::getInputHandler() -> InputFrame&
{
    return inputHandler.getRootFrame();
}
