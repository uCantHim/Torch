#pragma once

#include "trc/Types.h"

namespace trc::ui::event
{
    struct EventBase
    {
    public:
        inline void stopPropagate() {
            propagationStopped = true;
        }

        inline bool isPropagationStopped() const {
            return propagationStopped;
        }

    private:
        bool propagationStopped{ false };
    };

    struct MouseEvent : EventBase
    {
        vec2 mousePosPixels;
        vec2 mousePosNormal;
    };

    struct Click : MouseEvent {};
    struct Release : MouseEvent {};
    struct Hover : MouseEvent {};

    struct KeyPress : EventBase
    {
        KeyPress(int key) : key(key) {}
        int key;
    };

    struct KeyRelease : EventBase
    {
        KeyRelease(int key) : key(key) {}
        int key;
    };

    struct CharInput : EventBase
    {
        CharInput(ui32 c) : character(c) {}
        ui32 character;
    };
} // namespace trc::ui::event
