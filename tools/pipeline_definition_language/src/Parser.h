#pragma once

#include <initializer_list>
#include <memory>
#include <vector>
#include <variant>

#include "Token.h"
#include "SyntaxElements.h"

class ErrorReporter;

class Parser
{
public:
    Parser(std::vector<Token> tokens, ErrorReporter& errorReporter);

    auto parseTokens() -> ObjectDeclaration;

private:
    using Indent = Token::IndentLevel;

    auto parseFieldDef() -> FieldDefinition;
    auto parseFieldName() -> FieldName;
    auto parseFieldValue() -> FieldValue;
    auto parseObjectDecl() -> ObjectDeclaration;
    auto parseMatchExpr() -> MatchExpression;
    auto parseMatchCase() -> MatchCase;

    void error(const Token& token, std::string message);

    bool isAtEnd() const;
    auto consume() -> Token;
    auto previous() const -> const Token&;
    auto peek() const -> const Token&;
    bool check(TokenType type) const;
    bool match(std::initializer_list<TokenType> types);
    void expect(TokenType type, std::string errorMessage);

    bool matchCurrentIndent();
    void increaseIndentLevel();
    void decreaseIndentLevel();

    std::vector<Token> tokens;
    size_t current{ 0 };

    Indent currentIndent{ 0 };

    ErrorReporter* errorReporter;
};
