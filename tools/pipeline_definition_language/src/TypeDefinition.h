#pragma once

#include <string>
#include <unordered_set>
#include <variant>

#include "Token.h"

struct EnumTypeDef
{
    EnumTypeDef(Token token) : token(token), name(token.lexeme) {}

    Token token;

    std::string name;
    std::unordered_set<std::string> options;
};

using TypeDef = std::variant<
    EnumTypeDef
>;
