#include "InputProcessor.h"

#include <trc/base/Swapchain.h>

#include "viewport/ViewportTree.h"



//////////////////////////////////
//  Event processing functions  //
//////////////////////////////////

void InputProcessor::onKeyInput(
    trc::Swapchain& swapchain,
    trc::Key key,
    trc::InputAction action,
    trc::KeyModFlags mods)
{
    globalInputState.keyboard.notify(key, action);

    if (auto target = getTarget(swapchain)) {
        target->notify({ key, mods, action });
    }
}

void InputProcessor::onMouseInput(
    trc::Swapchain& swapchain,
    trc::MouseButton button,
    trc::InputAction action,
    trc::KeyModFlags mods)
{
    globalInputState.mouse.notify(button, action);

    if (auto target = getTarget(swapchain)) {
        target->notify({ button, mods, action });
    }
}

void InputProcessor::onMouseMove(trc::Swapchain& swapchain, double x, double y)
{
    globalInputState.mouse.notifyCursorMove({ x, y });

    if (auto target = getTarget(swapchain))
    {
        const vec2 newPos{ x, y };
        const vec2 diff = newPos - target->previousCursorPos;
        target->previousCursorPos = newPos;
        target->notify(CursorMovement{
            .position=newPos,
            .offset=diff,
            .areaSize=swapchain.getWindowSize()
        });
    }
}

void InputProcessor::onMouseScroll(trc::Swapchain& swapchain, double xOff, double yOff)
{
    if (auto target = getTarget(swapchain))
    {
        const vec2 scroll{ xOff, yOff };
        target->notify(Scroll{ .offset=scroll, .mod=swapchain.getKeyModifierState() });
    }
}

void InputProcessor::onMouseEnter(trc::Swapchain& swapchain, bool entered)
{
    if (entered) {
        hoveredWindow = &swapchain;
    }
    else if (hoveredWindow == &swapchain) {
        hoveredWindow = nullptr;
    }
}

void InputProcessor::onWindowFocus(trc::Swapchain& swapchain, bool focused)
{
    if (focused) {
        focusedWindow = &swapchain;
    }
    else if (focusedWindow == &swapchain) {
        focusedWindow = nullptr;
    }
}

void InputProcessor::onWindowResize(trc::Swapchain& window, uint x, uint y)
{
    if (auto target = getTarget(window)) {
        target->viewport->resize({ { 0, 0 }, { x, y } });
    }
}



///////////////////////////////////
//  Window management functions  //
///////////////////////////////////

auto InputProcessor::getGlobalInputState() -> const InputState&
{
    return globalInputState;
}

auto InputProcessor::getHoveredWindow() -> trc::Swapchain*
{
    return hoveredWindow;
}

auto InputProcessor::getFocusedWindow() -> trc::Swapchain*
{
    return focusedWindow;
}

auto InputProcessor::getRootViewport(trc::Swapchain& window) -> s_ptr<Viewport>
{
    if (auto target = getTarget(window)) {
        return target->viewport;
    }
    return nullptr;
}

auto InputProcessor::getInputState(trc::Swapchain& window) -> s_ptr<const InputState>
{
    if (auto target = getTarget(window)) {
        return target->inputState;
    }
    return nullptr;
}

void InputProcessor::setRootViewport(trc::Swapchain& window, s_ptr<Viewport> rootViewport)
{
    auto [it, isNewWindow] = windows.try_emplace(&window);
    if (isNewWindow)
    {
        it->second = std::make_unique<WindowState>();
        it->second->inputState = std::make_shared<InputState>();
        it->second->previousCursorPos = window.getMousePosition();
    }

    it->second->viewport = std::move(rootViewport);
}

auto InputProcessor::getTarget(trc::Swapchain& window) -> WindowState*
{
    auto it = windows.find(&window);
    if (it == windows.end()) {
        return nullptr;
    }
    return it->second.get();
}



void InputProcessor::WindowState::notify(const UserInput& input)
{
    if (viewport != nullptr) {
        viewport->notify(input);
    }
}

void InputProcessor::WindowState::notify(const Scroll& scroll)
{
    if (viewport != nullptr) {
        viewport->notify(scroll);
    }
}

void InputProcessor::WindowState::notify(const CursorMovement& cursorMove)
{
    if (viewport != nullptr) {
        viewport->notify(cursorMove);
    }
}
