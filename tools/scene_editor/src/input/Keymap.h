#pragma once

#include <cstdint>
#include <unordered_map>

#include <vkb/event/Keys.h>
#include <trc_util/data/IndexMap.h>

enum class InputAction : uint32_t
{
    eCameraRotateLeft = 0,
    eCameraRotateRight,
    eCameraRotateUp,
    eCameraRotateDown,

    NUM_INPUT_ACTIONS
};

constexpr auto CAMERA_ROTATE_BUTTON = vkb::MouseButton::right;
constexpr auto CAMERA_MOVE_BUTTON = vkb::MouseButton::left;

struct KeyBinding
{
    vkb::Key key;
    int mods{ 0 };
};

class Keymap
{
public:
    using Config = std::unordered_map<InputAction, KeyBinding>;

    static void bindKey(InputAction action, KeyBinding binding);
    static auto getKey(InputAction action) -> vkb::Key;
    static auto getKeyBinding(InputAction action) -> KeyBinding;

    /**
     * Result is equivalent to
     *
     *     int requiredMods = Keymap::getKeyBinding(action).mods;
     *     return pressedKey == Keymap::getKey(action)
     *            && (requiredMods & activeMods) == requiredMods;
     *
     * @return bool True if the given action is satisfied by the specified
     *              key and mods.
     */
    static bool isSatisfied(InputAction action, vkb::Key pressedKey, int activeMods);

private:
    static inline trc::data::IndexMap<size_t, KeyBinding> keyMap;
};

static const Keymap::Config defaultKeyConfig{
    { InputAction::eCameraRotateLeft, { vkb::Key::h } },
};
