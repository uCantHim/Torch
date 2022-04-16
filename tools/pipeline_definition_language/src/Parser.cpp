#include "Parser.h"

#include <cassert>

#include "Exceptions.h"
#include "ErrorReporter.h"
#include "FieldValue.h"



Parser::Parser(std::vector<Token> _tokens, ErrorReporter& errorReporter)
    :
    tokens(std::move(_tokens)),
    errorReporter(&errorReporter)
{
    assert(!tokens.empty() && "Token stream must have at least the EOF token");

    // Requirement to the scanner: All newlines must be followed by an indent token
#ifndef NDEBUG
    for (size_t i = 0; i < tokens.size(); ++i)
    {
        if (tokens.at(i).type == TokenType::eNewline)
        {
            assert(tokens.size() > i + 1 && tokens.at(i + 1).type == TokenType::eIndent
                   && "A token of type eNewline must always be followed by one of type eIndent!");
        }
    }
#endif
}

auto Parser::parseTokens() -> ObjectDeclaration
{
    try {
        return parseObjectDecl();
    }
    catch (const ParseError&) {
        return {};
    }
}

auto Parser::parseFieldDef() -> FieldDefinition
{
    auto name = parseFieldName();
    if (!match({ TokenType::eColon }))
    {
        error(peek(), "Expected ':' after field name.");
    }
    auto value = parseFieldValue();

    return { .name=std::move(name), .value=std::move(value) };
}

auto Parser::parseFieldName() -> std::variant<TypelessFieldName, TypedFieldName>
{
    if (!match({ TokenType::eIdentifier }))
    {
        error(peek(), "Expected identifier as a field name.");
    }

    if (check(TokenType::eIdentifier))
    {
        return TypedFieldName{
            .type{ previous().lexeme },
            .name{ consume().lexeme },
        };
    }

    return TypelessFieldName{ .name{ previous().lexeme } };
}

auto Parser::parseFieldValue() -> std::unique_ptr<FieldValue>
{
    switch (peek().type)
    {
    case TokenType::eLiteralString:
        return [this]{
            auto result = std::make_unique<LiteralFieldValue>(
                LiteralValue{ .value=std::get<Token::StringValue>(consume().value) }
            );
            expect(TokenType::eNewline, "Expected newline after literal value.");
            return result;
        }();
        break;
    case TokenType::eIdentifier:
        return [this]{
            auto result = std::make_unique<IdentifierFieldValue>(
                Identifier{ .name=consume().lexeme }
            );
            expect(TokenType::eNewline, "Expected newline after identifier.");
            return result;
        }();
        break;
    case TokenType::eNewline:
        return std::make_unique<ObjectDeclarationFieldValue>(parseObjectDecl());
        break;
    case TokenType::eMatch:
        return std::make_unique<MatchExpressionFieldValue>(parseMatchExpr());
        break;
    default:
        // Error
        error(peek(), "Expected a field value");
        break;
    }

    throw std::logic_error("The program can never reach this throw");
}

auto Parser::parseObjectDecl() -> ObjectDeclaration
{
    expect(TokenType::eNewline, "Expected newline to begin an object declaration.");

    ObjectDeclaration obj;
    increaseIndentLevel();
    while (matchCurrentIndent() && !isAtEnd())
    {
        try {
            obj.fields.emplace_back(parseFieldDef());
        }
        catch (const ParseError&) {
            break;  // End this object declaration
        }
    }
    decreaseIndentLevel();

    if (obj.fields.empty()) {
        error(peek(), "Expected at least one field.");
    }

    return obj;
}

auto Parser::parseMatchExpr() -> MatchExpression
{
    expect(TokenType::eMatch, "Expected keyword \"match\" to start match expression.");
    expect(TokenType::eIdentifier, "Expected identifier after \"match\".");

    MatchExpression expr{ .matchedType{ previous().lexeme } };
    expect(TokenType::eNewline, "Expected newline after match identifier.");

    increaseIndentLevel();
    while (!isAtEnd() && matchCurrentIndent()) {
        expr.cases.emplace_back(parseMatchCase());
    }
    decreaseIndentLevel();

    if (expr.cases.empty()) {
        error(peek(), "Expected at least one match case.");
    }

    return expr;
}

auto Parser::parseMatchCase() -> MatchCase
{
    expect(TokenType::eIdentifier, "Expected identifier at the beginning of the match case.");
    auto identifier = previous().lexeme;
    expect(TokenType::eRightArrow, "Expected RIGHT_ARROW ('->') in case expression.");
    auto fieldValue = parseFieldValue();

    return MatchCase{
        .caseIdentifier={ std::move(identifier) },
        .value=std::move(fieldValue),
    };
}

void Parser::error(const Token& token, std::string message)
{
    errorReporter->error(Error{
        .location=token.location,
        .message="At token " + to_string(token.type) + ": " + std::move(message)
    });
    throw ParseError{};
}

bool Parser::isAtEnd() const
{
    assert(tokens.size() > current);
    return tokens.at(current).type == TokenType::eEof;
}

auto Parser::previous() const -> const Token&
{
    return tokens.at(current - 1);
}

auto Parser::consume() -> Token
{
    if (!isAtEnd()) ++current;
    return previous();
}

auto Parser::peek() const -> const Token&
{
    return tokens.at(current);
}

bool Parser::check(TokenType type) const
{
    if (isAtEnd()) return false;
    return peek().type == type;
}

bool Parser::match(std::initializer_list<TokenType> types)
{
    for (TokenType type : types)
    {
        if (check(type))
        {
            consume();
            return true;
        }
    }

    return false;
}

void Parser::expect(TokenType type, std::string errorMessage)
{
    if (!match({ type })) {
        error(peek(), std::move(errorMessage));
    }
}

bool Parser::matchCurrentIndent()
{
    if (peek().type != TokenType::eIndent) {
        error(peek(), "<internal> Expected indent token.");
    }

    // Increase indent by one so that the indentation level can be 0
    // initially and increased to 1 for members of the implicit global
    // object.
    const Indent indent = std::get<Indent>(peek().value) + 1;
    if (indent == currentIndent)
    {
        consume();
        return true;
    }

    if (indent > currentIndent) {
        error(peek(), "Unexpected indent.");
    }

    return false;
}

void Parser::increaseIndentLevel()
{
    ++currentIndent;
}

void Parser::decreaseIndentLevel()
{
    --currentIndent;
    assert(currentIndent >= 0);
}
