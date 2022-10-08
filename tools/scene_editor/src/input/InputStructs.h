#pragma once

#include <variant>

#include <trc/base/event/Keys.h>
#include <trc/Types.h>
using namespace trc::basic_types;

struct KeyInput
{
    KeyInput(trc::Key key);
    KeyInput(trc::Key key, trc::KeyModFlags mods);
    KeyInput(trc::Key key, trc::KeyModFlags mods, trc::InputAction action);

    trc::Key key;
    trc::KeyModFlags mod{ trc::KeyModFlagBits::none };
    trc::InputAction action{ trc::InputAction::press };
};

struct MouseInput
{
    MouseInput(trc::MouseButton button);
    MouseInput(trc::MouseButton button, trc::KeyModFlags mods);
    MouseInput(trc::MouseButton button, trc::KeyModFlags mods, trc::InputAction action);

    trc::MouseButton button;
    trc::KeyModFlags mod{ trc::KeyModFlagBits::none };
    trc::InputAction action{ trc::InputAction::press };
};

/**
 * @brief Can be either MouseInput or KeyInput
 *
 * A std::hash specialization is defined which invokes the correct hash
 * function for MouseInput or KeyInput.
 */
struct VariantInput
{
    template<typename T>
    static constexpr bool keyOrMouseInput = std::same_as<T, KeyInput> || std::same_as<T, MouseInput>;

    VariantInput(KeyInput in) : input(in) {}
    VariantInput(MouseInput in) : input(in) {}

    VariantInput(trc::Key key);
    VariantInput(trc::Key key, trc::KeyModFlags mods);
    VariantInput(trc::Key key, trc::KeyModFlags mods, trc::InputAction action);

    VariantInput(trc::MouseButton button);
    VariantInput(trc::MouseButton button, trc::KeyModFlags mods);
    VariantInput(trc::MouseButton button, trc::KeyModFlags mods, trc::InputAction action);

    template<typename T> requires keyOrMouseInput<T>
    bool is() const
    {
        return std::holds_alternative<T>(input);
    }

    template<typename T> requires keyOrMouseInput<T>
    auto get() const -> T
    {
        if (!is<T>()) throw trc::Exception("Input is not of the requested type");
        return std::get<T>(input);
    }

    std::variant<KeyInput, MouseInput> input;
};



inline bool operator==(const KeyInput& a, const KeyInput& b)
{
    return a.key == b.key && a.mod == b.mod && a.action == b.action;
}

inline bool operator==(const MouseInput& a, const MouseInput& b)
{
    return a.button == b.button && a.mod == b.mod && a.action == b.action;
}

template<>
struct std::hash<KeyInput>
{
    inline auto operator()(const KeyInput& val) const noexcept
    {
        const ui32 h = (static_cast<ui32>(val.action) << 29)
                     | (static_cast<ui32>(val.key) << 16)
                     | (static_cast<ui8>(val.mod));
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
                     | (static_cast<ui8>(val.mod));
        return hash<ui32>{}(h);
    }
};

template<>
struct std::hash<VariantInput>
{
    inline auto operator()(const VariantInput& val) const noexcept
    {
        return std::visit(
            [](auto&& val) { return std::hash<std::decay_t<decltype(val)>>{}(val); },
            val.input
        );
    }
};
