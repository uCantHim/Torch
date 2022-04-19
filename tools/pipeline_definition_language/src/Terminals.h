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
