#pragma once

#include <stdexcept>
#include <string>
#include <variant>

enum class TokenType
{
    // Keywords
    eMatch, eEnum,

    // Symbols
    eIndent, eNewline, eColon, eRightArrow, eComma,

    // Literals
    eIdentifier, eLiteralString,

    // End of stream
    eEof,
};

inline auto to_string(TokenType type) -> std::string
{
    switch (type)
    {
    case TokenType::eMatch:
        return "eMatch";
    case TokenType::eEnum:
        return "eEnum";
    case TokenType::eIndent:
        return "eIndent";
    case TokenType::eNewline:
        return "eNewline";
    case TokenType::eColon:
        return "eColon";
    case TokenType::eRightArrow:
        return "eRightArrow";
    case TokenType::eComma:
        return "eComma";
    case TokenType::eIdentifier:
        return "eIdentifier";
    case TokenType::eLiteralString:
        return "eLiteralString";
    case TokenType::eEof:
        return "eEof";
    }

    throw std::logic_error("");
}

struct TokenLocation
{
    size_t line;
    size_t start;
    size_t length;
};

struct Token
{
    using IndentLevel = size_t;
    using StringValue = std::string;
    using Value = std::variant<std::monostate, IndentLevel, StringValue>;

    TokenType type;
    std::string lexeme;
    Value value;

    TokenLocation location;
};
