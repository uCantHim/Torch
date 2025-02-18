#pragma once

#include <unordered_map>

#include <trc/Types.h>
using namespace trc::basic_types;

#include "Command.h"
#include "InputStructs.h"

/**
 * Could be turned into a template
 */
class KeyMap
{
public:
    KeyMap() = default;

    auto get(const UserInput& input) -> Command*;

    void set(const UserInput& input, u_ptr<Command> cmd);
    void unset(const UserInput& input);

    void clear();

private:
    std::unordered_map<UserInput, u_ptr<Command>> map{};
};
