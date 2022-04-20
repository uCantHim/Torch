#pragma once

#include <string>

#include "Token.h"

/**
 * We only have string literal types
 */
struct LiteralValue
{
    LiteralValue(Token _token)
        : token(std::move(_token)), value(std::get<Token::StringValue>(token.value))
    {}

    Token token;
    std::string value;
};

/**
 * @brief An identifier is just an arbitrary string name
 */
struct Identifier
{
    Identifier(Token _token)
        : token(std::move(_token)), name(token.lexeme)
    {}

    Token token;
    std::string name;
};

inline bool operator==(const Identifier& a, const Identifier& b)
{
    return a.name == b.name;
}

namespace std
{
    template<>
    struct hash<Identifier>
    {
        auto operator()(const Identifier& id) const {
            return std::hash<std::string>{}(id.name);
        }
    };
} // namespace std
