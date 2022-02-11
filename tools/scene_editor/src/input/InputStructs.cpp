#include "InputStructs.h"



KeyInput::KeyInput(vkb::Key key)
    : key(key)
{
}

KeyInput::KeyInput(vkb::Key key, vkb::KeyModFlags mods)
    : key(key), mod(mods)
{
}

KeyInput::KeyInput(vkb::Key key, vkb::KeyModFlags mods, vkb::InputAction action)
    : key(key), mod(mods), action(action)
{
}

MouseInput::MouseInput(vkb::MouseButton button)
    : button(button)
{
}

MouseInput::MouseInput(vkb::MouseButton button, vkb::KeyModFlags mods)
    : button(button), mod(mods)
{
}

MouseInput::MouseInput(vkb::MouseButton button, vkb::KeyModFlags mods, vkb::InputAction action)
    : button(button), mod(mods), action(action)
{
}



VariantInput::VariantInput(vkb::Key key)
    : input(KeyInput(key))
{
}

VariantInput::VariantInput(vkb::Key key, vkb::KeyModFlags mods)
    : input(KeyInput(key, mods))
{
}

VariantInput::VariantInput(vkb::Key key, vkb::KeyModFlags mods, vkb::InputAction action)
    : input(KeyInput(key, mods, action))
{
}

VariantInput::VariantInput(vkb::MouseButton button)
    : input(MouseInput(button))
{
}

VariantInput::VariantInput(vkb::MouseButton button, vkb::KeyModFlags mods)
    : input(MouseInput(button, mods))
{
}

VariantInput::VariantInput(vkb::MouseButton button, vkb::KeyModFlags mods, vkb::InputAction action)
    : input(MouseInput(button, mods, action))
{
}
