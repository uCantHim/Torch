#pragma once

#include <variant>

#include <trc/base/event/Keys.h>
#include <trc/Types.h>
#include <trc_util/Exception.h>
using namespace trc::basic_types;

/**
 * @brief Defines a keyboard input.
 *
 * Represents one specific key input combination, such as 'A press',
 * 'ctrl-C press', or 'alt-TAB release'.
 */
struct KeyInput
{
    KeyInput(trc::Key key);
    KeyInput(trc::Key key, trc::KeyModFlags mods);
    KeyInput(trc::Key key, trc::InputAction action);
    KeyInput(trc::Key key, trc::KeyModFlags mods, trc::InputAction action);

    bool operator==(const KeyInput&) const = default;

    trc::Key key;
    trc::KeyModFlags mods{ trc::KeyModFlagBits::none };
    trc::InputAction action{ trc::InputAction::press };
};

/**
 * @brief Defines a mouse input.
 *
 * Represents one specific mouse input combination, such as 'left button release',
 * or 'right button press with ctrl-shift'.
 */
struct MouseInput
{
    MouseInput(trc::MouseButton button);
    MouseInput(trc::MouseButton button, trc::KeyModFlags mods);
    MouseInput(trc::MouseButton button, trc::InputAction action);
    MouseInput(trc::MouseButton button, trc::KeyModFlags mods, trc::InputAction action);

    bool operator==(const MouseInput&) const = default;

    trc::MouseButton button;
    trc::KeyModFlags mods{ trc::KeyModFlagBits::none };
    trc::InputAction action{ trc::InputAction::press };
};

/**
 * @brief Information about a scroll input.
 */
struct Scroll
{
    // The scroll offset. Quantifies how much scroll has been applied.
    //
    // Normal mouse input produces scroll offset along the y-axis, i.e.,
    // `scroll.y`. Other input methods, such as touchpad swipes, may generate
    // scroll offsets on the x-axis as well.
    vec2 offset;

    // Key modifiers active at the time of scrolling.
    trc::KeyModFlags mod{ trc::KeyModFlagBits::none };
};

/**
 * @brief Information about a cursor movement input.
 */
struct CursorMovement
{
    // Current cursor position in screen coordinates, relative to the top-left
    // corner of the window.
    vec2 position;

    // How far the cursor has been moved since the last 'cursor move' event in
    // units of screen coordinates.
    vec2 offset;

    // Size of the area on which the cursor was moved in pixels. This is usually
    // the window size, but may be something like the size of the viewport on
    // which the cursor event occurred, depending on the implementation.
    //
    // `position` is relative to the origin coordinates `{0, 0}` of this area.
    //
    // The following statements are always true:
    // `0 <= position.x < area.x`
    // `0 <= position.y < area.y`
    uvec2 areaSize;
};

/**
 * @brief Can be either MouseInput or KeyInput
 *
 * A std::hash specialization is defined which invokes the correct hash
 * function for MouseInput or KeyInput.
 */
struct UserInput
{
    template<typename T>
    static constexpr bool keyOrMouseInput = std::same_as<T, KeyInput> || std::same_as<T, MouseInput>;

    constexpr UserInput(KeyInput in) : input(in) {}
    constexpr UserInput(MouseInput in) : input(in) {}

    UserInput(trc::Key key);
    UserInput(trc::Key key, trc::KeyModFlags mods);
    UserInput(trc::Key key, trc::KeyModFlags mods, trc::InputAction action);

    UserInput(trc::MouseButton button);
    UserInput(trc::MouseButton button, trc::KeyModFlags mods);
    UserInput(trc::MouseButton button, trc::KeyModFlags mods, trc::InputAction action);

    bool operator==(const UserInput& other) const = default;

    std::variant<KeyInput, MouseInput> input;
};

template<>
struct std::hash<KeyInput>
{
    inline auto operator()(const KeyInput& val) const noexcept
    {
        const ui32 h = (static_cast<ui32>(val.action) << 29)
                     | (static_cast<ui32>(val.key) << 16)
                     | (static_cast<ui8>(val.mods));
        return hash<ui32>{}(h);
    }
};

template<>
struct std::hash<MouseInput>
{
    inline auto operator()(const MouseInput& val) const noexcept
    {
        const ui32 h = (static_cast<ui32>(val.action) << 29)
                     | (static_cast<ui32>(val.button) << 8)
                     | (static_cast<ui8>(val.mods));
        return hash<ui32>{}(h);
    }
};

template<>
struct std::hash<UserInput>
{
    inline auto operator()(const UserInput& val) const
    {
        return std::visit(
            [](auto&& val) { return std::hash<std::decay_t<decltype(val)>>{}(val); },
            val.input
        );
    }
};
