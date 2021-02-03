#pragma once

#include "Types.h"

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
} // namespace trc::ui
