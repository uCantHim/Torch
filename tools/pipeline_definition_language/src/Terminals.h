#pragma once

#include <string>
#include <variant>

#include "Token.h"

struct StringLiteral
{
    StringLiteral(Token _token)
        : token(std::move(_token)), value(std::get<Token::StringValue>(token.value))
    {}

    Token token;
    std::string value;
};

struct NumberLiteral
{
    using NumberValue = Token::NumberValue;

    NumberLiteral(Token _token)
        : token(std::move(_token)), value(std::get<Token::NumberValue>(token.value))
    {}

    Token token;
    NumberValue value;
};

using LiteralValue = std::variant<
    StringLiteral,
    NumberLiteral
>;

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
