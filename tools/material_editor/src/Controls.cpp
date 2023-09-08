#include "Controls.h"

#include <trc/base/event/Event.h>
#include <trc/base/event/InputState.h>
#include <trc/core/Window.h>



struct MouseDragEvent
{
    vec2 initialMousePos;
    vec2 currentMousePos;

    vec2 dragIncrement;
};

struct ZoomEvent
{
    // 0 indicates no deviation from default zoom.
    // Negative values mean zoomed in, positive ones zoomed out.
    i32 zoomLevel{ 0 };
};

void registerMouseDragEventGenerator()
{
    constexpr trc::MouseButton kDragButton{ trc::MouseButton::left };
    constexpr ui32 kFramesUntilDrag{ 5 };

    static ui32 framesMoved{ 0 };
    static vec2 initialMousePos{ 0, 0 };
    static vec2 currentMousePos{ 0, 0 };

    trc::on<trc::MouseClickEvent>([](const auto& e) {
        if (e.button == kDragButton)
        {
            framesMoved = 0;
            initialMousePos = trc::Mouse::getPosition();
            currentMousePos = initialMousePos;
        }
    });
    trc::on<trc::MouseReleaseEvent>([](const auto& e) {
        if (e.button == kDragButton) {
            framesMoved = 0;
        }
    });
    trc::on<trc::MouseMoveEvent>([](const auto&) {
        if (trc::Mouse::isPressed(kDragButton))
        {
            ++framesMoved;
            if (framesMoved >= kFramesUntilDrag)
            {
                const vec2 inc = trc::Mouse::getPosition() - currentMousePos;
                currentMousePos = trc::Mouse::getPosition();
                trc::fire(MouseDragEvent{ initialMousePos, currentMousePos, inc });
            }
        }
    });
}

void registerZoomEventGenerator()
{
    static i32 zoomLevel{ 0 };

    trc::on<trc::ScrollEvent>([](const trc::ScrollEvent& e) {
        zoomLevel += e.yOffset < 0.0f ? 1 : -1;
        trc::fire(ZoomEvent{ zoomLevel });
    });
}



MaterialEditorControls::MaterialEditorControls(
    trc::Window& window,
    MaterialEditorGui& gui,
    trc::Camera& camera)
{
    constexpr float kZoomStep{ 0.15f };

    trc::Keyboard::init();
    trc::Mouse::init();

    registerMouseDragEventGenerator();
    registerZoomEventGenerator();

    // Scale camera drag distance by size of the orthogonal camera
    static vec2 cameraSize{ 1.0f, 1.0f };

    trc::on<MouseDragEvent>([&](auto&& e) {
        const vec2 move = (e.dragIncrement * vec2(1, -1)) / vec2(window.getSize());
        camera.translate(vec3(move * cameraSize, 0.0f));
    });
    trc::on<ZoomEvent>([&camera](const ZoomEvent& e) {
        const float min = 0.0f - static_cast<float>(e.zoomLevel) * kZoomStep;
        const float max = 1.0f + static_cast<float>(e.zoomLevel) * kZoomStep;
        camera.makeOrthogonal(min, max, min, max, -10.0f, 10.0f);
        cameraSize = { max - min, max - min };
    });
}

void MaterialEditorControls::update()
{
}
