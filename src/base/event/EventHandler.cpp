#include "trc/base/event/EventHandler.h"

#include <stdexcept>

#include "trc/base/Logging.h"
#include "trc/base/event/InputEvents.h"


namespace trc
{

void InputEventSpawner::onCharInput(Swapchain& swapchain, uint32_t charcode)
{
    EventHandler<CharInputEvent>::notify({ &swapchain, charcode });
}

void InputEventSpawner::onKeyInput(
    Swapchain& swapchain,
    Key key,
    InputAction action,
    KeyModFlags mods)
{
    switch(action)
    {
    case InputAction::press:
        EventHandler<KeyPressEvent>::notify({ &swapchain, key, mods });
        break;
    case InputAction::release:
        EventHandler<KeyReleaseEvent>::notify({ &swapchain, key, mods });
        break;
    case InputAction::repeat:
        EventHandler<KeyRepeatEvent>::notify({ &swapchain, key, mods });
        break;
    default:
        throw std::logic_error("");
    };
}

void InputEventSpawner::onMouseInput(
    Swapchain& swapchain,
    MouseButton button,
    InputAction action,
    KeyModFlags mods)
{
    switch (action)
    {
    case InputAction::press:
        EventHandler<MouseClickEvent>::notify({ &swapchain, button, mods });
        break;
    case InputAction::release:
        EventHandler<MouseReleaseEvent>::notify({ &swapchain, button, mods });
        break;
    default:
        throw std::logic_error("");
    }
}

void InputEventSpawner::onMouseMove(Swapchain& swapchain, double x, double y)
{
    EventHandler<MouseMoveEvent>::notify(
        { &swapchain, static_cast<float>(x), static_cast<float>(y) }
    );
}

void InputEventSpawner::onMouseScroll(Swapchain& swapchain, double xOffset, double yOffset)
{
    EventHandler<ScrollEvent>::notify(
        { &swapchain, static_cast<float>(xOffset), static_cast<float>(yOffset) }
    );
}

void InputEventSpawner::onWindowClose(Swapchain& swapchain)
{
    EventHandler<SwapchainCloseEvent>::notify({ &swapchain });
}



void EventThread::start()
{
    terminate();
    shouldStop = false;

    thread = std::thread([]()
    {
        while (!shouldStop) {
            pollFuncs.wait_pop()();
        }
    });

    log::info << "--- Event thread started";
}

void EventThread::terminate()
{
    shouldStop = true;
    if (thread.joinable())
    {
        notifyActiveHandler([]{});
        thread.join();
    }
}

void EventThread::notifyActiveHandler(void(*pollFunc)())
{
    pollFuncs.push(pollFunc);
}

} // namespace trc
