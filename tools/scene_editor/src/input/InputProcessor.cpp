#include "InputProcessor.h"

#include <trc/ImguiIntegration.h>
#include <trc/base/Swapchain.h>
#include <trc_util/Assert.h>

#include "input/EventTarget.h"
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

void InputProcessor::onCharInput(trc::Swapchain& swapchain, uint32_t c)
{
    auto res = EventTarget::NotifyResult::eRejected;
    if (res == EventTarget::NotifyResult::eRejected) {
        trc::imgui::impl_callback::charInput(swapchain, c);
    }
}

void InputProcessor::onKeyInput(
    trc::Swapchain& swapchain,
    trc::Key key,
    trc::InputAction action,
    trc::KeyModFlags mods)
{
    globalState.keyboard.notify(key, action);
    auto res = rootViewport->notify({ key, mods, action });

    if (res == EventTarget::NotifyResult::eRejected) {
        trc::imgui::impl_callback::key(swapchain, key, action, mods);
    }
}

void InputProcessor::onMouseEnter(trc::Swapchain& swapchain, bool entered)
{
    auto res = EventTarget::NotifyResult::eRejected;
    if (res == EventTarget::NotifyResult::eRejected) {
        trc::imgui::impl_callback::cursorEnter(swapchain, entered);
    }
}

void InputProcessor::onMouseInput(
    trc::Swapchain& swapchain,
    trc::MouseButton button,
    trc::InputAction action,
    trc::KeyModFlags mods)
{
    globalState.mouse.notify(button, action);
    auto res = rootViewport->notify({ button, mods, action });

    // TODO: Theoretically, it would be much cleaner to move these calls to the
    // respective ImguiWindow::notify function. This would require ImguiWindow
    // to have a handle to its window. Someone should implement this.
    if (res == EventTarget::NotifyResult::eRejected) {
        trc::imgui::impl_callback::mouseButton(swapchain, button, action, mods);
    }
}

void InputProcessor::onMouseMove(trc::Swapchain& swapchain, double x, double y)
{
    globalState.mouse.notifyCursorMove({ x, y });

    const vec2 newPos{ x, y };
    const vec2 diff = newPos - previousCursorPos;
    previousCursorPos = newPos;
    auto res = rootViewport->notify(CursorMovement{
        .position=newPos,
        .offset=diff,
        .areaSize=swapchain.getWindowSize()
    });

    if (res == EventTarget::NotifyResult::eRejected) {
        trc::imgui::impl_callback::cursorPos(swapchain, x, y);
    }
}

void InputProcessor::onMouseScroll(trc::Swapchain& swapchain, double xOff, double yOff)
{
    const vec2 scroll{ xOff, yOff };
    auto res = rootViewport->notify(Scroll{ .offset=scroll, .mod=swapchain.getKeyModifierState() });

    if (res == EventTarget::NotifyResult::eRejected) {
        trc::imgui::impl_callback::scroll(swapchain, xOff, yOff);
    }
}

void InputProcessor::onWindowFocus(trc::Swapchain& swapchain, bool focused)
{
    auto res = EventTarget::NotifyResult::eRejected;
    if (res == EventTarget::NotifyResult::eRejected) {
        trc::imgui::impl_callback::windowFocus(swapchain, focused);
    }
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
