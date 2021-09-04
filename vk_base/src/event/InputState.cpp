#include "event/InputState.h"

#include "event/EventHandler.h"
#include "event/InputEvents.h"



void vkb::Keyboard::init()
{
    EventHandler<KeyPressEvent>::addListener([](const auto& e) {
        states[static_cast<size_t>(e.key)] = InputAction::press;
    });

    EventHandler<KeyReleaseEvent>::addListener([](const auto& e) {
        states[static_cast<size_t>(e.key)] = InputAction::release;
    });
}

bool vkb::Keyboard::isPressed(Key key)
{
    return states[static_cast<size_t>(key)] == InputAction::press;
}

bool vkb::Keyboard::isReleased(Key key)
{
    return states[static_cast<size_t>(key)] == InputAction::release;
}

auto vkb::Keyboard::getState(Key key) -> InputAction
{
    return states[static_cast<size_t>(key)];
}



void vkb::Mouse::init()
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

bool vkb::Mouse::isPressed(MouseButton button)
{
    return states[static_cast<size_t>(button)] == InputAction::press;
}

bool vkb::Mouse::isReleased(MouseButton button)
{
    return states[static_cast<size_t>(button)] == InputAction::release;
}

auto vkb::Mouse::getState(MouseButton button) -> InputAction
{
    return states[static_cast<size_t>(button)];
}

auto vkb::Mouse::getPosition() -> glm::vec2
{
    return mousePos;
}
