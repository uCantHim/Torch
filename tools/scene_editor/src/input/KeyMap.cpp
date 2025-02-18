#include "KeyMap.h"



auto KeyMap::get(const UserInput& input) -> Command*
{
    auto it = map.find(input);
    if (it != map.end()) {
        return it->second.get();
    }
    return nullptr;
}

void KeyMap::set(const UserInput& input, u_ptr<Command> cmd)
{
    assert(cmd != nullptr);

    auto [it, success] = map.try_emplace(input);
    it->second = std::move(cmd);
}

void KeyMap::unset(const UserInput& input)
{
    map.erase(input);
}

void KeyMap::clear()
{
    map.clear();
}
