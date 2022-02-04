#include "KeyMap.h"



KeyInput::KeyInput(vkb::Key key)
    : key(key)
{
}

KeyInput::KeyInput(vkb::Key key, vkb::KeyModFlags mods)
    : key(key), mod(mods)
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



auto KeyMap::get(KeyInput input) -> InputCommand*
{
    auto it = map.find(std::hash<KeyInput>{}(input));
    if (it != map.end()) {
        return it->second.get();
    }
    return nullptr;
}

auto KeyMap::get(MouseInput input) -> InputCommand*
{
    auto it = map.find(std::hash<MouseInput>{}(input));
    if (it != map.end()) {
        return it->second.get();
    }
    return nullptr;
}

void KeyMap::set(KeyInput input, u_ptr<InputCommand> cmd)
{
    assert(cmd != nullptr);

    auto [it, success] = map.try_emplace(std::hash<KeyInput>{}(input));
    it->second = std::move(cmd);
}

void KeyMap::set(MouseInput input, u_ptr<InputCommand> cmd)
{
    assert(cmd != nullptr);

    auto [it, success] = map.try_emplace(std::hash<MouseInput>{}(input));
    it->second = std::move(cmd);
}

void KeyMap::unset(KeyInput input)
{
    map.erase(std::hash<KeyInput>{}(input));
}

void KeyMap::unset(MouseInput input)
{
    map.erase(std::hash<MouseInput>{}(input));
}

void KeyMap::clear()
{
    map.clear();
}
