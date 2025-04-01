#include "InputProcessor.h"

#include <trc/base/Swapchain.h>
#include <trc_util/Assert.h>

#include "input/UserInput.h"



//////////////////////////////////
//  Event processing functions  //
//////////////////////////////////

InputProcessor::InputProcessor(s_ptr<Viewport> _rootViewport)
    :
    rootViewport(std::move(_rootViewport))
{
    assert_arg(rootViewport != nullptr);
}

void InputProcessor::onKeyInput(
    trc::Swapchain&,
    trc::Key key,
    trc::InputAction action,
    trc::KeyModFlags mods)
{
    globalState.keyboard.notify(key, action);
    rootViewport->notify({ key, mods, action });
}

void InputProcessor::onMouseInput(
    trc::Swapchain&,
    trc::MouseButton button,
    trc::InputAction action,
    trc::KeyModFlags mods)
{
    globalState.mouse.notify(button, action);
    rootViewport->notify({ button, mods, action });
}

void InputProcessor::onMouseMove(trc::Swapchain& swapchain, double x, double y)
{
    globalState.mouse.notifyCursorMove({ x, y });

    const vec2 newPos{ x, y };
    const vec2 diff = newPos - previousCursorPos;
    previousCursorPos = newPos;
    rootViewport->notify(CursorMovement{
        .position=newPos,
        .offset=diff,
        .areaSize=swapchain.getWindowSize()
    });
}

void InputProcessor::onMouseScroll(trc::Swapchain& swapchain, double xOff, double yOff)
{
    const vec2 scroll{ xOff, yOff };
    rootViewport->notify(Scroll{ .offset=scroll, .mod=swapchain.getKeyModifierState() });
}

void InputProcessor::onWindowResize(trc::Swapchain&, uint x, uint y)
{
    rootViewport->resize({ { 0, 0 }, { x, y } });
}

auto InputProcessor::getRootViewport() -> s_ptr<Viewport>
{
    return rootViewport;
}

auto InputProcessor::getGlobalInputState() -> const GlobalInputState&
{
    return globalState;
}
