#include "input/MouseState.h"



void MouseState::notify(trc::MouseButton button, trc::InputAction action)
{
    states[idx(button)] = action;
}

void MouseState::notifyCursorMove(vec2 newPos)
{
    mousePos = newPos;
}

bool MouseState::isPressed(trc::MouseButton button) const
{
    return states[idx(button)] == trc::InputAction::press;
}

bool MouseState::isReleased(trc::MouseButton button) const
{
    return states[idx(button)] == trc::InputAction::release;
}

auto MouseState::getState(trc::MouseButton button) const -> trc::InputAction
{
    return states[idx(button)];
}

auto MouseState::getCursorPos() const -> vec2
{
    return mousePos;
}
