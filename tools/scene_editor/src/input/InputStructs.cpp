#include "InputStructs.h"



KeyInput::KeyInput(trc::Key key)
    : key(key)
{
}

KeyInput::KeyInput(trc::Key key, trc::KeyModFlags mods)
    : key(key), mods(mods)
{
}

KeyInput::KeyInput(trc::Key key, trc::InputAction action)
    : key(key), action(action)
{
}

KeyInput::KeyInput(trc::Key key, trc::KeyModFlags mods, trc::InputAction action)
    : key(key), mods(mods), action(action)
{
}

MouseInput::MouseInput(trc::MouseButton button)
    : button(button)
{
}

MouseInput::MouseInput(trc::MouseButton button, trc::KeyModFlags mods)
    : button(button), mods(mods)
{
}

MouseInput::MouseInput(trc::MouseButton button, trc::InputAction action)
    : button(button), action(action)
{
}

MouseInput::MouseInput(trc::MouseButton button, trc::KeyModFlags mods, trc::InputAction action)
    : button(button), mods(mods), action(action)
{
}



UserInput::UserInput(trc::Key key)
    : input(KeyInput(key))
{
}

UserInput::UserInput(trc::Key key, trc::KeyModFlags mods)
    : input(KeyInput(key, mods))
{
}

UserInput::UserInput(trc::Key key, trc::KeyModFlags mods, trc::InputAction action)
    : input(KeyInput(key, mods, action))
{
}

UserInput::UserInput(trc::MouseButton button)
    : input(MouseInput(button))
{
}

UserInput::UserInput(trc::MouseButton button, trc::KeyModFlags mods)
    : input(MouseInput(button, mods))
{
}

UserInput::UserInput(trc::MouseButton button, trc::KeyModFlags mods, trc::InputAction action)
    : input(MouseInput(button, mods, action))
{
}
