#pragma once

#include "Keys.h"
#include "../basics/Swapchain.h"

namespace vkb
{
    ///////////////////////////
    //      Key Events       //
    ///////////////////////////

    struct KeyEventBase
    {
        Swapchain* swapchain;
        Key key;
        KeyModFlags mods;
        InputAction action;

    protected:
        KeyEventBase(Swapchain* sc, Key key, KeyModFlags mod, InputAction action)
            : swapchain(sc), key(key), mods(mod), action(action) {}
    };

    struct KeyPressEvent : public KeyEventBase
    {
        KeyPressEvent(Swapchain* sc, Key key, KeyModFlags mod)
            : KeyEventBase(sc, key, mod, InputAction::press) {}

    };

    struct KeyReleaseEvent : public KeyEventBase
    {
        KeyReleaseEvent(Swapchain* sc, Key key, KeyModFlags mod)
            : KeyEventBase(sc, key, mod, InputAction::release) {}
    };

    struct KeyRepeatEvent : public KeyEventBase
    {
        KeyRepeatEvent(Swapchain* sc, Key key, KeyModFlags mod)
            : KeyEventBase(sc, key, mod, InputAction::repeat) {}
    };


    struct CharInputEvent
    {
        Swapchain* swapchain;
        uint32_t character;
    };


    /////////////////////////////
    //      Mouse Events       //
    /////////////////////////////

    struct MouseMoveEvent
    {
        Swapchain* swapchain;
        float x;
        float y;
    };

    struct MouseButtonEventBase
    {
        Swapchain* swapchain;
        MouseButton button;
        KeyModFlags mods;
        InputAction action;

    protected:
        MouseButtonEventBase(Swapchain* sc, MouseButton button, KeyModFlags mods, InputAction action)
            : swapchain(sc), button(button), mods(mods), action(action) {}
    };

    struct MouseClickEvent : public MouseButtonEventBase
    {
        MouseClickEvent(Swapchain* sc, MouseButton button, KeyModFlags mods)
            : MouseButtonEventBase(sc, button, mods, InputAction::press) {}
    };

    struct MouseReleaseEvent : public MouseButtonEventBase
    {
        MouseReleaseEvent(Swapchain* sc, MouseButton button, KeyModFlags mods)
            : MouseButtonEventBase(sc, button, mods, InputAction::release) {}
    };


    /////////////////////////////
    //      Other Events       //
    /////////////////////////////

    struct ScrollEvent
    {
        Swapchain* swapchain;
        float xOffset;
        float yOffset;
    };
}
