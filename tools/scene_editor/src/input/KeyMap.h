#pragma once

#include <unordered_map>

#include <trc/Types.h>
using namespace trc::basic_types;

#include "InputCommand.h"
#include "InputStructs.h"

/**
 * Could be turned into a template
 */
class KeyMap
{
public:
    KeyMap() = default;

    auto get(UserInput input) -> InputCommand*;

    void set(UserInput input, u_ptr<InputCommand> cmd);
    void unset(UserInput input);

    void clear();

private:
    std::unordered_map<UserInput, u_ptr<InputCommand>> map{};
};
