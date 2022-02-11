#include "KeyMap.h"



auto KeyMap::get(VariantInput input) -> InputCommand*
{
    auto it = map.find(std::hash<VariantInput>{}(input));
    if (it != map.end()) {
        return it->second.get();
    }
    return nullptr;
}

void KeyMap::set(VariantInput input, u_ptr<InputCommand> cmd)
{
    assert(cmd != nullptr);

    auto [it, success] = map.try_emplace(std::hash<VariantInput>{}(input));
    it->second = std::move(cmd);
}

void KeyMap::unset(VariantInput input)
{
    map.erase(std::hash<VariantInput>{}(input));
}

void KeyMap::clear()
{
    map.clear();
}
