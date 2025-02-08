#pragma once

#include "trc/base/Swapchain.h"
#include "trc/base/event/Keys.h"

namespace trc
{
    struct SwapchainFocusEvent
    {
        Swapchain* swapchain;
        bool focused;
    };

    struct SwapchainResizeEvent
    {
        Swapchain* swapchain;
        uvec2 newSize;
    };

    struct SwapchainMoveEvent
    {
        Swapchain* swapchain;
        ivec2 newPos;
    };

    /**
     * Dispatched when the OS requests the window to close.
     */
    struct SwapchainCloseEvent
    {
        Swapchain* swapchain;
    };

    /**
     * Dispatched when the swapchain's content has become damaged and needs to
     * be refreshed.
     */
    struct SwapchainRefreshEvent
    {
        Swapchain* swapchain;
    };

    /**
     * Dispatched when a window gains or loses focus.
     */
    using WindowFocusEvent = SwapchainFocusEvent;

    /**
     * Dispatched when the window is resized.
     */
    using WindowResizeEvent = SwapchainResizeEvent;

    /**
     * Dispatched when the window is moved to a new position on the screen.
     */
    using WindowMoveEvent = SwapchainMoveEvent;

    /**
     * Dispatched when the user attempts to close a window.
     */
    using WindowCloseEvent = SwapchainCloseEvent;

    /**
     * Dispatched when the window's content has become damaged and needs to
     * be refreshed.
     */
    using WindowRefreshEvent = SwapchainRefreshEvent;

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

    /**
     * Dispatched when the cursor enters or leaves a window.
     */
    struct MouseEnterEvent
    {
        Swapchain* swapchain;

        // True if the cursor entered the window, false if it left.
        bool entered;
    };

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
