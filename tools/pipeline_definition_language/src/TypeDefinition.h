#pragma once

#include <string>
#include <unordered_set>
#include <variant>

#include "Token.h"

struct EnumOption
{
    EnumOption(Token _token) : token(std::move(_token)), value(token.lexeme) {}

    Token token;
    std::string value;
};

inline bool operator==(const EnumOption& a, const EnumOption& b)
{
    return a.value == b.value;
}

namespace std
{
    template<>
    struct hash<EnumOption>
    {
        auto operator()(const EnumOption& opt) const {
            return std::hash<std::string>{}(opt.value);
        }
    };
} // namespace std



struct EnumTypeDef
{
    EnumTypeDef(Token _token) : token(std::move(_token)), name(token.lexeme) {}

    Token token;

    std::string name;
    std::unordered_set<EnumOption> options;
};

using TypeDef = std::variant<
    EnumTypeDef
>;
