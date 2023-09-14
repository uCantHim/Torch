#include "trc/base/event/InputState.h"

#include "trc/base/event/EventHandler.h"
#include "trc/base/event/InputEvents.h"



void trc::Keyboard::init()
{
    EventHandler<KeyPressEvent>::addListener([](const auto& e) {
        states[static_cast<size_t>(e.key)] = InputAction::press;
        firstTimePressed[static_cast<size_t>(e.key)] = true;
    });

    EventHandler<KeyReleaseEvent>::addListener([](const auto& e) {
        states[static_cast<size_t>(e.key)] = InputAction::release;
        firstTimeReleased[static_cast<size_t>(e.key)] = true;
    });
}

bool trc::Keyboard::isPressed(Key key)
{
    return states[static_cast<size_t>(key)] == InputAction::press;
}

bool trc::Keyboard::isReleased(Key key)
{
    return states[static_cast<size_t>(key)] == InputAction::release;
}

bool trc::Keyboard::wasPressed(Key key)
{
    const bool res = firstTimePressed[static_cast<size_t>(key)];
    firstTimePressed[static_cast<size_t>(key)] = false;
    return res;
}

bool trc::Keyboard::wasReleased(Key key)
{
    const bool res = firstTimeReleased[static_cast<size_t>(key)];
    firstTimeReleased[static_cast<size_t>(key)] = false;
    return res;
}

auto trc::Keyboard::getState(Key key) -> InputAction
{
    return states[static_cast<size_t>(key)];
}



void trc::Mouse::init()
{
    EventHandler<MouseClickEvent>::addListener([](const auto& e) {
        states[static_cast<size_t>(e.button)] = InputAction::press;
        firstTimePressed[static_cast<size_t>(e.button)] = true;
    });
    EventHandler<MouseReleaseEvent>::addListener([](const auto& e) {
        states[static_cast<size_t>(e.button)] = InputAction::release;
        firstTimeReleased[static_cast<size_t>(e.button)] = true;
    });

    EventHandler<MouseMoveEvent>::addListener([](const MouseMoveEvent& e) {
        mousePos = glm::vec2{ e.x, e.y };
        firstTimeMoved = true;
    });
}

bool trc::Mouse::isPressed(MouseButton button)
{
    return states[static_cast<size_t>(button)] == InputAction::press;
}

bool trc::Mouse::isReleased(MouseButton button)
{
    return states[static_cast<size_t>(button)] == InputAction::release;
}

bool trc::Mouse::wasPressed(MouseButton button)
{
    const bool res = firstTimePressed[static_cast<size_t>(button)];
    firstTimePressed[static_cast<size_t>(button)] = false;
    return res;
}

bool trc::Mouse::wasReleased(MouseButton button)
{
    const bool res = firstTimeReleased[static_cast<size_t>(button)];
    firstTimeReleased[static_cast<size_t>(button)] = false;
    return res;
}

auto trc::Mouse::getState(MouseButton button) -> InputAction
{
    return states[static_cast<size_t>(button)];
}

auto trc::Mouse::getPosition() -> glm::vec2
{
    return mousePos;
}

bool trc::Mouse::wasMoved()
{
    const bool res = firstTimeMoved;
    firstTimeMoved = false;
    return res;
}
