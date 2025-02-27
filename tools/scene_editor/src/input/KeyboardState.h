#pragma once

#include <array>

#include <trc/base/event/Keys.h>

class KeyboardState
{
public:
    void notify(trc::Key key, trc::InputAction action);

    /**
     * @return bool True if the key is currently pressed.
     */
    bool isPressed(trc::Key key) const;

    /**
     * @return bool True if the key is currently not pressed.
     */
    bool isReleased(trc::Key key) const;

    /**
     * @return The current state of key `key`. Is either `press` or `release`.
     */
    auto getState(trc::Key key) const -> trc::InputAction;

    /**
     * @return All currently active key modifiers.
     */
    auto getActiveMods() const -> trc::KeyModFlags;

private:
    static constexpr size_t kNumKeys = static_cast<size_t>(trc::Key::MAX_ENUM);
    static constexpr auto idx(trc::Key key) { return static_cast<size_t>(key); }

    std::array<bool, kNumKeys> pressed{ false };
};
