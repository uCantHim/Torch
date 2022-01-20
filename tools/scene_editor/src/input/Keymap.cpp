#include "Keymap.h"



void Keymap::bindKey(InputAction action, KeyBinding binding)
{
    keyMap[static_cast<size_t>(action)] = binding;
}

auto Keymap::getKey(InputAction action) -> vkb::Key
{
    return keyMap[static_cast<size_t>(action)].key;
}

auto Keymap::getKeyBinding(InputAction action) -> KeyBinding
{
    return keyMap[static_cast<size_t>(action)];
}

bool Keymap::isSatisfied(InputAction action, vkb::Key pressedKey, int activeMods)
{
    auto binding = getKeyBinding(action);
    return pressedKey == binding.key && (activeMods & binding.mods) == binding.mods;
}
