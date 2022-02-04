#pragma once

#include <initializer_list>
#include <variant>
#include <unordered_map>

#include <trc/Types.h>
using namespace trc::basic_types;

#include "InputStructs.h"
#include "InputCommand.h"

/**
 * Could be turned into a template
 */
class KeyMap
{
public:
    KeyMap() = default;

    auto get(KeyInput input) -> InputCommand*;
    auto get(MouseInput input) -> InputCommand*;

    void set(KeyInput input, u_ptr<InputCommand> cmd);
    void set(MouseInput input, u_ptr<InputCommand> cmd);
    void unset(KeyInput input);
    void unset(MouseInput input);
    void clear();

private:
    using CommonHashType = decltype(std::hash<ui32>{}(ui32{}));

    std::unordered_map<CommonHashType, u_ptr<InputCommand>> map{};
};
