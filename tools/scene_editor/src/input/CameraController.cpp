#include "CameraController.h"



input::CameraController::CameraController(trc::Camera& camera)
    :
    camera(&camera)
{
    mouseClickListener = vkb::on<vkb::MouseClickEvent>(
        [this](const auto& e)
        {
            if (e.button == CAMERA_ROTATE_BUTTON || e.button == CAMERA_MOVE_BUTTON) {
                lastMousePos = vkb::Mouse::getPosition();
            }
        }
    );
    mouseMoveListener = vkb::on<vkb::MouseMoveEvent>(
        [this](const auto& e)
        {
            if (vkb::Mouse::isPressed(CAMERA_ROTATE_BUTTON))
            {
                const vec2 newMousePos{ e.x, e.y };
                lastMousePos = newMousePos;
            }
        }
    );
}
