#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "Token.h"

class ErrorReporter;

class Scanner
{
public:
    Scanner(std::string source, ErrorReporter& errorReporter);

    auto scanTokens() -> std::vector<Token>;

private:
    bool isAtEnd() const;
    void scanToken();
    void scanStringLiteral();
    void scanIdentifier();
    void scanIndent();

    void error(std::string message);

    auto peek() -> char;
    auto consume() -> char;
    bool match(char c);

    static bool isAlpha(char c);
    static bool isDigit(char c);
    static bool isAlphaNumeric(char c);

    void addToken(TokenType type);
    void addToken(TokenType type, Token::Value value);

    std::string source;
    std::vector<Token> tokens;

    size_t start{ 0 };
    size_t current{ 0 };
    size_t line{ 1 };

    static const std::unordered_map<std::string, TokenType> keywords;

    ErrorReporter* errorReporter;
};
