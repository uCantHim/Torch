#include "Parser.h"

#include <cassert>

#include "Exceptions.h"
#include "ErrorReporter.h"



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

auto Parser::parseTokens() -> std::vector<Stmt>
{
    while (match(TokenType::eNewline));

    try {
        std::vector<Stmt> result;
        while (matchCurrentIndent() && !isAtEnd()) {
            result.emplace_back(parseStatement());
        }

        return result;
    }
    catch (const ParseError&) {
        return {};
    }
}

auto Parser::parseStatement() -> Stmt
{
    switch (peek().type)
    {
    case TokenType::eIdentifier:
        return parseFieldDef();
    case TokenType::eEnum:
        return parseEnum();
    default:
        error(peek(), "Expected type definition or field definition.");
    }

    throw std::logic_error("This code can never be reached.");
}

auto Parser::parseEnum() -> EnumTypeDef
{
    expect(TokenType::eEnum, "Expected ENUM to start enum definition.");
    expect(TokenType::eIdentifier, "Expected identifier.");

    EnumTypeDef def{ previous() };
    expect(TokenType::eColon, "Expected COLON after enum identifier.");
    match(TokenType::eNewline);

    // Parse options
    increaseIndentLevel();
    while ((check(TokenType::eIdentifier) || matchCurrentIndent()) && !isAtEnd())
    {
        expect(TokenType::eIdentifier, "Expected enum option.");
        auto [it, success] = def.options.emplace(previous());
        if (!success) {
            error(previous(), "Redefinition of enum option \"" + it->value + "\".");
        }

        // The last option is allowed to omit the comma
        const bool hadComma = match(TokenType::eComma);
        match(TokenType::eNewline);

        if (!hadComma) break;
    }
    decreaseIndentLevel();

    if (def.options.empty()) {
        error(peek(), "Expected at least one option for enum \"" + def.name + "\".");
    }

    return def;
}

auto Parser::parseFieldDef() -> FieldDefinition
{
    auto name = parseFieldName();
    if (!match(TokenType::eColon))
    {
        error(peek(), "Expected ':' after field name.");
    }
    auto value = parseFieldValue();
    expectNewline("Expected newline after field definition");

    return { .name=std::move(name), .value=std::make_unique<FieldValue>(std::move(value)) };
}

auto Parser::parseFieldName() -> std::variant<TypelessFieldName, TypedFieldName>
{
    if (!match(TokenType::eIdentifier))
    {
        error(peek(), "Expected identifier as a field name.");
    }

    if (check(TokenType::eIdentifier))
    {
        return TypedFieldName{
            .name{ previous() },
            .mappedName{ consume() },
        };
    }

    return TypelessFieldName{ .name{ previous() } };
}

auto Parser::parseFieldValue() -> FieldValue
{
    switch (peek().type)
    {
    case TokenType::eLiteralString:
        return StringLiteral{ consume() };
    case TokenType::eLiteralNumber:
        return NumberLiteral{ consume() };
    case TokenType::eIdentifier:
        return Identifier{ consume() };
    case TokenType::eLeftBracket:
        return parseListDecl();
    case TokenType::eNewline:
        return parseObjectDecl();
    case TokenType::eMatch:
        return parseMatchExpr();
    default:
        // Error
        error(peek(), "Expected a field value");
        break;
    }

    throw std::logic_error("The program can never reach this throw");
}

auto Parser::parseListDecl() -> ListDeclaration
{
    expect(TokenType::eLeftBracket, "Expected LEFT_BRACKET to begin a list declaration");

    ListDeclaration list{ previous() };
    while (!match(TokenType::eRightBracket))
    {
        match(TokenType::eNewline);  // Optional
        match(TokenType::eIndent);  // Optional

        list.items.emplace_back(parseFieldValue());

        const bool hadComma = match(TokenType::eComma);
        match(TokenType::eNewline);  // Optional
        match(TokenType::eIndent);  // Optional
        if (!hadComma)
        {
            expect(TokenType::eRightBracket, "Expected comma to separate list entries.");
            break;
        }
    }

    return list;
}

auto Parser::parseObjectDecl() -> ObjectDeclaration
{
    expect(TokenType::eNewline, "Expected newline to begin an object declaration.");

    ObjectDeclaration obj{ previous() };
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

    MatchExpression expr{ previous() };
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
    Identifier identifier{ previous() };
    expect(TokenType::eRightArrow, "Expected RIGHT_ARROW ('->') in case expression.");
    auto fieldValue = parseFieldValue();
    expectNewline("Expected newline after match case.");

    return MatchCase{
        .caseIdentifier={ std::move(identifier) },
        .value=std::make_unique<FieldValue>(std::move(fieldValue)),
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

bool Parser::match(TokenType type)
{
    if (check(type))
    {
        consume();
        return true;
    }

    return false;
}

bool Parser::match(std::initializer_list<TokenType> types)
{
    for (TokenType type : types) {
        if (match(type)) return true;
    }

    return false;
}

void Parser::expect(TokenType type, std::string errorMessage)
{
    if (!match(type)) {
        error(peek(), std::move(errorMessage));
    }
}

bool Parser::matchNewline()
{
    return previous().type == TokenType::eNewline || match(TokenType::eNewline);
}

void Parser::expectNewline(std::string errorMessage)
{
    if (!matchNewline()) {
        error(peek(), std::move(errorMessage));
    }
}

auto Parser::peekIndent() -> Indent
{
    if (peek().type != TokenType::eIndent) {
        error(peek(), "Expected indent.");
    }

    return std::get<Indent>(peek().value);
}

bool Parser::matchCurrentIndent()
{
    const Indent indent = peekIndent();
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
