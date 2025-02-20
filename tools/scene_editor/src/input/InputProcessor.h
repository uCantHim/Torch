#pragma once

#include <trc/Types.h>
#include <trc/base/InputProcessor.h>
using namespace trc::basic_types;

#include "input/EventTarget.h"

/**
 * @brief The scene editor's implementation of an input processor.
 *
 * Dispatches events to the correct viewport, etc.
 */
class InputProcessor : public trc::InputProcessor
{
public:
    explicit InputProcessor(s_ptr<EventTarget> target);

    void onCharInput(trc::Swapchain&, uint32_t) override {}
    void onKeyInput(trc::Swapchain&, trc::Key, trc::InputAction, trc::KeyModFlags) override;
    void onMouseEnter(trc::Swapchain&, bool) override {}
    void onMouseInput(trc::Swapchain&, trc::MouseButton, trc::InputAction, trc::KeyModFlags) override;
    void onMouseMove(trc::Swapchain&, double, double) override;
    void onMouseScroll(trc::Swapchain&, double, double) override;

    void onWindowFocus(trc::Swapchain&, bool) override {}
    void onWindowResize(trc::Swapchain&, uint, uint) override {}
    void onWindowClose(trc::Swapchain&) override {}
    void onWindowMove(trc::Swapchain&, int, int) override {}
    void onWindowRefresh(trc::Swapchain&) override {}

private:
    s_ptr<EventTarget> target;

    vec2 previousCursorPos;

    /*
    struct EventTarget  // <- Viewport?
    {
        vec2 previousCursorPos;

        InputStateMachine inputState;
    };

    auto getTarget(trc::Swapchain& swapchain) -> EventTarget&;

    std::unordered_map<trc::Swapchain*, EventTarget> eventTargets;
    */
};
