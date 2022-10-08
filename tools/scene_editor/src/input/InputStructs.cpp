#include "InputStructs.h"



KeyInput::KeyInput(trc::Key key)
    : key(key)
{
}

KeyInput::KeyInput(trc::Key key, trc::KeyModFlags mods)
    : key(key), mod(mods)
{
}

KeyInput::KeyInput(trc::Key key, trc::KeyModFlags mods, trc::InputAction action)
    : key(key), mod(mods), action(action)
{
}

MouseInput::MouseInput(trc::MouseButton button)
    : button(button)
{
}

MouseInput::MouseInput(trc::MouseButton button, trc::KeyModFlags mods)
    : button(button), mod(mods)
{
}

MouseInput::MouseInput(trc::MouseButton button, trc::KeyModFlags mods, trc::InputAction action)
    : button(button), mod(mods), action(action)
{
}



VariantInput::VariantInput(trc::Key key)
    : input(KeyInput(key))
{
}

VariantInput::VariantInput(trc::Key key, trc::KeyModFlags mods)
    : input(KeyInput(key, mods))
{
}

VariantInput::VariantInput(trc::Key key, trc::KeyModFlags mods, trc::InputAction action)
    : input(KeyInput(key, mods, action))
{
}

VariantInput::VariantInput(trc::MouseButton button)
    : input(MouseInput(button))
{
}

VariantInput::VariantInput(trc::MouseButton button, trc::KeyModFlags mods)
    : input(MouseInput(button, mods))
{
}

VariantInput::VariantInput(trc::MouseButton button, trc::KeyModFlags mods, trc::InputAction action)
    : input(MouseInput(button, mods, action))
{
}
