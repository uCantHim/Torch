#pragma once

#include <array>

#include <trc/Types.h>
#include <trc/base/event/Keys.h>
using namespace trc::basic_types;

class MouseState
{
public:
    void notify(trc::MouseButton button, trc::InputAction action);
    void notifyCursorMove(vec2 newPos);

    bool isPressed(trc::MouseButton button) const;

    bool isReleased(trc::MouseButton button) const;

    auto getState(trc::MouseButton button) const -> trc::InputAction;

    /**
     * @return Current cursor position relative to the upper-left corner in
     *         pixels.
     */
    auto getCursorPos() const -> vec2;

private:
    static constexpr size_t kNumButtons = static_cast<size_t>(trc::MouseButton::MAX_ENUM);
    static constexpr auto idx(trc::MouseButton b) { return static_cast<size_t>(b); }

    std::array<trc::InputAction, kNumButtons> states{ trc::InputAction::release };
    vec2 mousePos;
};
