#include "InputProcessor.h"

#include <trc/base/Swapchain.h>



InputProcessor::InputProcessor(s_ptr<EventTarget> _target)
    :
    target(std::move(_target))
{
}

void InputProcessor::onKeyInput(
    trc::Swapchain&,
    trc::Key key,
    trc::InputAction action,
    trc::KeyModFlags mods)
{
    target->notify({ key, mods, action });
}

void InputProcessor::onMouseInput(
    trc::Swapchain&,
    trc::MouseButton button,
    trc::InputAction action,
    trc::KeyModFlags mods)
{
    target->notify({ button, mods, action });
}

void InputProcessor::onMouseMove(trc::Swapchain& swapchain, double x, double y)
{
    const vec2 newPos{ x, y };
    const vec2 diff = newPos - previousCursorPos;
    previousCursorPos = newPos;
    target->notify(CursorMovement{
        .position=newPos,
        .offset=diff,
        .areaSize=swapchain.getWindowSize()
    });
}

void InputProcessor::onMouseScroll(trc::Swapchain& swapchain, double xOff, double yOff)
{
    const vec2 scroll{ xOff, yOff };
    target->notify(Scroll{ .offset=scroll, .mod=swapchain.getKeyModifierState() });
}
