#pragma once

#include <stdexcept>
#include <string>
#include <variant>

enum class TokenType
{
    // Keywords
    eMatch, eEnum, eImport,

    // Symbols
    eIndent, eNewline,
    eColon, eRightArrow, eComma,
    eLeftBracket, eRightBracket,

    // Literals
    eIdentifier, eLiteralString, eLiteralNumber,

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
    case TokenType::eImport:
        return "eImport";
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
    case TokenType::eLeftBracket:
        return "eLeftBracket";
    case TokenType::eRightBracket:
        return "eRightBracket";
    case TokenType::eIdentifier:
        return "eIdentifier";
    case TokenType::eLiteralString:
        return "eLiteralString";
    case TokenType::eLiteralNumber:
        return "eLiteralNumber";
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
    using IndentSpaces = size_t;
    using StringValue = std::string;
    using NumberValue = std::variant<double, int64_t>;
    using Value = std::variant<std::monostate, IndentSpaces, StringValue, NumberValue>;

    TokenType type;
    std::string lexeme;
    Value value;

    TokenLocation location;
};
