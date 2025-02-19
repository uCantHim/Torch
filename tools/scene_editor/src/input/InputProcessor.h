#pragma once

#include <trc/base/InputProcessor.h>

#include "input/InputState.h"

/**
 * @brief The scene editor's implementation of an input processor.
 *
 * Dispatches events to the correct viewport, etc.
 */
class InputProcessor : public trc::InputProcessor
{
public:
    // InputProcessor(ViewportTree* targets);
    explicit InputProcessor(u_ptr<InputFrame> rootFrame);

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
    InputStateMachine inputState;

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
