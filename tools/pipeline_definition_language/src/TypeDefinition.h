#pragma once

#include <string>
#include <unordered_set>
#include <variant>

struct EnumTypeDef
{
    std::string name;
    std::unordered_set<std::string> options;
};

using TypeDef = std::variant<
    EnumTypeDef
>;
