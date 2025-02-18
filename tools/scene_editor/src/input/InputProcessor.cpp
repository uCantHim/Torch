#include "InputProcessor.h"

#include <trc/base/Swapchain.h>



InputProcessor::InputProcessor(u_ptr<InputFrame> rootFrame)
    :
    inputState(std::move(rootFrame))
{
}

void InputProcessor::onKeyInput(
    trc::Swapchain&,
    trc::Key key,
    trc::InputAction action,
    trc::KeyModFlags mods)
{
    // auto& vp = findViewportAt(swapchain.getMousePosition());
    // vp.notify({ key, mods, action });

    inputState.notify({ key, mods, action });
}

void InputProcessor::onMouseInput(
    trc::Swapchain&,
    trc::MouseButton button,
    trc::InputAction action,
    trc::KeyModFlags mods)
{
    inputState.notify({ button, mods, action });
}

void InputProcessor::onMouseMove(trc::Swapchain&, double x, double y)
{
    const vec2 newPos{ x, y };
    const vec2 diff = newPos - previousCursorPos;
    previousCursorPos = newPos;
    inputState.notify(CursorMovement{ .position=newPos, .offset=diff });
}

void InputProcessor::onMouseScroll(trc::Swapchain& swapchain, double xOff, double yOff)
{
    const vec2 scroll{ xOff, yOff };
    inputState.notify(Scroll{ .offset=scroll, .mod=swapchain.getKeyModifierState() });
}
