#include "KeyMap.h"



auto KeyMap::get(UserInput input) -> InputCommand*
{
    auto it = map.find(std::hash<UserInput>{}(input));
    if (it != map.end()) {
        return it->second.get();
    }
    return nullptr;
}

void KeyMap::set(UserInput input, u_ptr<InputCommand> cmd)
{
    assert(cmd != nullptr);

    auto [it, success] = map.try_emplace(std::hash<UserInput>{}(input));
    it->second = std::move(cmd);
}

void KeyMap::unset(UserInput input)
{
    map.erase(std::hash<UserInput>{}(input));
}

void KeyMap::clear()
{
    map.clear();
}
