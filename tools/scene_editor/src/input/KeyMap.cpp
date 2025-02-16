#include "KeyMap.h"



auto KeyMap::get(UserInput input) -> InputCommand*
{
    auto it = map.find(input);
    if (it != map.end()) {
        return it->second.get();
    }
    return nullptr;
}

void KeyMap::set(UserInput input, u_ptr<InputCommand> cmd)
{
    assert(cmd != nullptr);

    auto [it, success] = map.try_emplace(input);
    it->second = std::move(cmd);
}

void KeyMap::unset(UserInput input)
{
    map.erase(input);
}

void KeyMap::clear()
{
    map.clear();
}
