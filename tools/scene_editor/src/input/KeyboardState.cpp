#include "input/KeyboardState.h"



void KeyboardState::notify(trc::Key key, trc::InputAction action)
{
    pressed[idx(key)] = (action == trc::InputAction::press || action == trc::InputAction::repeat);
}

bool KeyboardState::isPressed(trc::Key key) const
{
    return pressed[idx(key)];
}

bool KeyboardState::isReleased(trc::Key key) const
{
    return !pressed[idx(key)];
}

auto KeyboardState::getState(trc::Key key) const -> trc::InputAction
{
    return pressed[idx(key)] ? trc::InputAction::press : trc::InputAction::release;
}

auto KeyboardState::getActiveMods() const -> trc::KeyModFlags
{
    /**
     * Fun fact: GCC 14.2 with -O1 compiles this to a branchless series of
     *
     * ```
     * movzx   edx, BYTE PTR states[...]
     * sal     edx, X
     * or      eax, edx
     * ```
     *
     * instructions. Additionally, because the key enums are laid out such that
     * the numerical values of the modifier keys are adjacent to each other, all
     * memory accesses will be on the same cache line. Nice.
     */

    constexpr auto none = trc::KeyModFlagBits::none;

    return
          (isPressed(trc::Key::left_shift)  ? trc::KeyModFlagBits::shift     : none)
        | (isPressed(trc::Key::right_shift) ? trc::KeyModFlagBits::shift     : none)
        | (isPressed(trc::Key::left_alt)    ? trc::KeyModFlagBits::alt       : none)
        | (isPressed(trc::Key::right_alt)   ? trc::KeyModFlagBits::alt       : none)
        | (isPressed(trc::Key::left_ctrl)   ? trc::KeyModFlagBits::control   : none)
        | (isPressed(trc::Key::right_ctrl)  ? trc::KeyModFlagBits::control   : none)
        | (isPressed(trc::Key::left_super)  ? trc::KeyModFlagBits::super     : none)
        | (isPressed(trc::Key::right_super) ? trc::KeyModFlagBits::super     : none)
        | (isPressed(trc::Key::caps_lock)   ? trc::KeyModFlagBits::caps_lock : none)
        | (isPressed(trc::Key::num_lock)    ? trc::KeyModFlagBits::num_lock  : none);
}
