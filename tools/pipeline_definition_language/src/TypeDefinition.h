#pragma once

#include <string>
#include <vector>
#include <variant>

struct EnumTypeDef
{
    std::string name;
    std::vector<std::string> options;
};

using TypeDef = std::variant<
    EnumTypeDef
>;
