#pragma once

#include "Types.h"

namespace trc::ui
{
    /**
     * For every member, set the value passed to Window::signalKeyPress
     * when the corresponding key is pressed.
     */
    struct KeyMapping
    {
        int escape;
        int backspace;
        int enter;
        int tab;
        int del;
    };

    struct IoConfig
    {
        // Stores the current state of keyboard keys
        bool keysDown[512]{ 0 };

        // Maps keys to indices in the `keysDown` array
        KeyMapping keyMap;
    };
} // namespace trc::ui
