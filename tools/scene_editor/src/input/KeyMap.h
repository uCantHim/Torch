#pragma once

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

    auto get(VariantInput input) -> InputCommand*;

    void set(VariantInput input, u_ptr<InputCommand> cmd);
    void unset(VariantInput input);

    void clear();

private:
    using CommonHashType = decltype(std::hash<ui32>{}(ui32{}));

    std::unordered_map<CommonHashType, u_ptr<InputCommand>> map{};
};
