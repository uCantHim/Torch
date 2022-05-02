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

    auto parseTokens() -> std::vector<Stmt>;

private:
    using Indent = Token::IndentSpaces;

    auto parseStatement() -> Stmt;

    auto parseEnum() -> EnumTypeDef;
    auto parseFieldDef() -> FieldDefinition;
    auto parseFieldName() -> FieldName;
    auto parseFieldValue() -> FieldValue;
    auto parseListDecl() -> ListDeclaration;
    auto parseObjectDecl() -> ObjectDeclaration;
    auto parseMatchExpr() -> MatchExpression;
    auto parseMatchCase() -> MatchCase;

    void error(const Token& token, std::string message);

    bool isAtEnd() const;
    auto consume() -> Token;
    auto previous() const -> const Token&;
    auto peek() const -> const Token&;
    bool check(TokenType type) const;
    bool match(TokenType type);
    bool match(std::initializer_list<TokenType> types);
    void expect(TokenType type, std::string errorMessage);

    /**
     * True if either the next or the previous token is a newline.
     * Consumes the newline in the first case.
     */
    bool matchNewline();

    /**
     * True if either the next or the previous token is a newline.
     * Consumes the newline in the first case.
     * This is necessary because multiple nested fields definitions end at
     * the same newline token.
     *
     * Issues an error of no newline is found.
     */
    void expectNewline(std::string errorMessage);

    auto peekIndent() -> Indent;
    bool matchCurrentIndent();
    void increaseIndentLevel();
    void decreaseIndentLevel();

    std::vector<Token> tokens;
    size_t current{ 0 };

    Indent currentIndent{ 0 };

    ErrorReporter* errorReporter;
};
