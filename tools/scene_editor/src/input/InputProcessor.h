#pragma once

#include <unordered_map>

#include <trc/Types.h>
#include <trc/base/InputProcessor.h>
using namespace trc::basic_types;

#include "input/InputStructs.h"
#include "input/KeyboardState.h"
#include "input/MouseState.h"
#include "viewport/Viewport.h"

/**
 * @brief Keeps track of the current state of input devices.
 */
struct InputState
{
    KeyboardState keyboard;
    MouseState mouse;
};

/**
 * @brief The scene editor's implementation of an input processor.
 *
 * Manages all windows created by the application and dispatches events to their
 * respective viewport managers.
 */
class InputProcessor : public trc::InputProcessor
{
public:
    void onCharInput(trc::Swapchain&, uint32_t) override {}
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
     * @return The current state of the input devices, as witnessed across all
     *         of the application's windows.
     */
    auto getGlobalInputState() -> const InputState&;

    /**
     * @return The window that is currently hovered by the cursor. `nullptr` if
     *         the cursor hovers none of the application's windows.
     */
    auto getHoveredWindow() -> trc::Swapchain*;

    /**
     * @return The window that is currently focused by the window system.
     *         `nullptr` if none of the application's windows are focused.
     */
    auto getFocusedWindow() -> trc::Swapchain*;

    /**
     * @param window The window whose viewport manager to obtain.
     *
     * @return The window's viewport manager. Can be `nullptr` if the window has
     *         no root viewport.
     */
    auto getRootViewport(trc::Swapchain& window) -> s_ptr<Viewport>;

    /**
     * @return State of input devices as witnessed by `window`.
     */
    auto getInputState(trc::Swapchain& window) -> s_ptr<const InputState>;

    /**
     * @brief Initialize a new window.
     *
     * @param window   The window to register at the window manager.
     * @param viewport The new root viewport for `window`. May be `nullptr` to
     *                 create a stub window.
     */
    void setRootViewport(trc::Swapchain& window, s_ptr<Viewport> rootViewport);

    /**
     * @brief Remove a window from the window manager.
     *
     * The window will no longer be tracked and will not receive input events.
     */
    void removeWindow(trc::Swapchain& window);

private:
    struct WindowState
    {
        void notify(const UserInput& input);
        void notify(const Scroll& scroll);
        void notify(const CursorMovement& cursorMove);

        s_ptr<Viewport> viewport;
        s_ptr<InputState> inputState;
        vec2 previousCursorPos;
    };

    auto getTarget(trc::Swapchain& window) -> WindowState*;

    std::unordered_map<trc::Swapchain*, u_ptr<WindowState>> windows;
    trc::Swapchain* focusedWindow{ nullptr };
    trc::Swapchain* hoveredWindow{ nullptr };

    InputState globalInputState;
};
