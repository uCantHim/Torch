#pragma once

#include <vkb/event/Event.h>
#include <trc/Camera.h>
using namespace glm;

#include "Keymap.h"

namespace input
{
    class CameraController
    {
    public:
        explicit CameraController(trc::Camera& camera);

    private:
        trc::Camera* const camera;
        vec2 lastMousePos{ 0.0f };

        vkb::UniqueListenerId<vkb::MouseClickEvent> mouseClickListener;
        vkb::UniqueListenerId<vkb::MouseMoveEvent> mouseMoveListener;
    };
} // namespace input
