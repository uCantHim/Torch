#include "trc/base/event/InputState.h"

#include "trc/base/event/EventHandler.h"
#include "trc/base/event/InputEvents.h"



void trc::Keyboard::init()
{
    EventHandler<KeyPressEvent>::addListener([](const auto& e) {
        states[static_cast<size_t>(e.key)] = InputAction::press;
    });

    EventHandler<KeyReleaseEvent>::addListener([](const auto& e) {
        states[static_cast<size_t>(e.key)] = InputAction::release;
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

auto trc::Keyboard::getState(Key key) -> InputAction
{
    return states[static_cast<size_t>(key)];
}



void trc::Mouse::init()
{
    EventHandler<MouseClickEvent>::addListener([](const auto& e) {
        states[static_cast<size_t>(e.button)] = InputAction::press;
    });
    EventHandler<MouseReleaseEvent>::addListener([](const auto& e) {
        states[static_cast<size_t>(e.button)] = InputAction::release;
    });

    EventHandler<MouseMoveEvent>::addListener([](const MouseMoveEvent& e) {
            mousePos = glm::vec2{ e.x, e.y };
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

auto trc::Mouse::getState(MouseButton button) -> InputAction
{
    return states[static_cast<size_t>(button)];
}

auto trc::Mouse::getPosition() -> glm::vec2
{
    return mousePos;
}
