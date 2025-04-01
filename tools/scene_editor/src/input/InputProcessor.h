#pragma once

#include <trc/Types.h>
#include <trc/base/InputProcessor.h>
using namespace trc::basic_types;

#include "input/GlobalInputState.h"
#include "viewport/Viewport.h"

/**
 * @brief The scene editor's implementation of an input processor.
 *
 * Dispatches relevant window events to a viewport.
 */
class InputProcessor : public trc::InputProcessor
{
public:
    explicit InputProcessor(s_ptr<Viewport> rootViewport);

    void onCharInput(trc::Swapchain&, uint32_t) override;
    void onKeyInput(trc::Swapchain&, trc::Key, trc::InputAction, trc::KeyModFlags) override;
    void onMouseEnter(trc::Swapchain&, bool) override;
    void onMouseInput(trc::Swapchain&, trc::MouseButton, trc::InputAction, trc::KeyModFlags) override;
    void onMouseMove(trc::Swapchain&, double, double) override;
    void onMouseScroll(trc::Swapchain&, double, double) override;

    void onWindowFocus(trc::Swapchain&, bool) override;
    void onWindowResize(trc::Swapchain&, uint, uint) override;
    void onWindowClose(trc::Swapchain&) override {}
    void onWindowMove(trc::Swapchain&, int, int) override {}
    void onWindowRefresh(trc::Swapchain&) override {}

    /**
     * @brief
     */
    auto getRootViewport() -> s_ptr<Viewport>;

    static auto getGlobalInputState() -> const GlobalInputState&;

private:
    static inline GlobalInputState globalState;

    s_ptr<Viewport> rootViewport;
    vec2 previousCursorPos{ 0, 0 };
};
