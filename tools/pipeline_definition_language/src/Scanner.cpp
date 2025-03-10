#include "Scanner.h"

#include <cmath>

#include "ErrorReporter.h"



const std::unordered_map<std::string, TokenType> Scanner::keywords{
    { "match", TokenType::eMatch },
    { "enum", TokenType::eEnum },
    { "import", TokenType::eImport },
};



Scanner::Scanner(std::string source, ErrorReporter& errorReporter)
    :
    source(std::move(source)),
    errorReporter(&errorReporter)
{
}

auto Scanner::scanTokens() -> std::vector<Token>
{
    addToken(TokenType::eNewline);
    scanIndent();
    while (!isAtEnd())
    {
        start = current;
        scanToken();
    }

    // Add EOF token
    tokens.emplace_back(Token{
        .type=TokenType::eEof,
        .lexeme="EOF",
        .value=std::monostate{},
        .location{
            .line=line,
            .start=0,
            .length=0,
        }
    });

    return tokens;
}

bool Scanner::isAtEnd() const
{
    return current == source.size();
}

void Scanner::scanToken()
{
    char c = consume();
    switch (c)
    {
    case ':': addToken(TokenType::eColon); break;
    case ',': addToken(TokenType::eComma); break;
    case '[': addToken(TokenType::eLeftBracket); break;
    case ']': addToken(TokenType::eRightBracket); break;
    case '-':
        if (match('>')) addToken(TokenType::eRightArrow);
        break;
    case '"':
        scanStringLiteral();
        break;
    case '/':
        if (match('/')) {
            while (!isAtEnd() && peek() != '\n') consume();
        }
        else error("Unexpected character '/'.");
        break;
    case ' ':
    case '\t':
    case '\r':
        break;
    case '\n':
        // Remove empty lines
        if (tokens.size() >= 2
            && tokens.at(tokens.size() - 1).type == TokenType::eIndent
            && tokens.at(tokens.size() - 2).type == TokenType::eNewline)
        {
            tokens.pop_back();
            tokens.pop_back();
        }
        addToken(TokenType::eNewline);
        ++line;
        start = current;
        scanIndent();
        break;
    default:
        if (isDigit(c)) {
            scanNumberLiteral(c);
        }
        else if (isAlpha(c)) {
            scanIdentifier();
        }
        else {
            error("Unexpected character: '" + std::to_string(c) + "'");
        }
        break;
    };
}

void Scanner::scanStringLiteral()
{
    while (peek() != '"' && !isAtEnd())
    {
        // Multiline strings are not allowed
        if (peek() == '\n')
        {
            error("Expected '\"', but encountered newline.");
            return;
        }

        // The next character is a valid character in the string, so consume it.
        consume();
    }

    if (isAtEnd())
    {
        error("Expected '\"', but encountered EOF.");
        return;
    }

    consume(); // Consume the closing " character

    const size_t len = current - start;
    std::string val = source.substr(start + 1, len - 2);
    addToken(TokenType::eLiteralString, std::move(val));
}

void Scanner::scanNumberLiteral(char firstDigit)
{
    int64_t preComma = firstDigit - '0';
    while (isDigit(peek()))
    {
        preComma = preComma * 10 + (consume() - '0');
    }

    if (match('.'))
    {
        double postComma = 0.0;
        size_t count = 1;
        while (isDigit(peek()))
        {
            postComma = postComma + (consume() - '0') * std::pow(0.1, count++);
        }
        addToken(TokenType::eLiteralNumber, double(preComma) + postComma);
    }
    else {
        addToken(TokenType::eLiteralNumber, preComma);
    }
}

void Scanner::scanIdentifier()
{
    while (isAlphaNumeric(peek())) consume();

    auto it = keywords.find(source.substr(start, current - start));
    if (it != keywords.end()) {
        addToken(it->second);
    }
    else {
        addToken(TokenType::eIdentifier);
    }
}

void Scanner::scanIndent()
{
    int spaces{ 0 };
    while (peek() == ' ')
    {
        ++spaces;
        consume();
    }

    if (peek() == '\t') {
        error("Tabs are not allowed as indent! Use spaces instead.");
    }

    addToken(TokenType::eIndent, Token::IndentSpaces(spaces));
}

void Scanner::error(std::string message)
{
    Token last = tokens.empty() ? Token{} : tokens.back();

    errorReporter->error({
        .location={ line, last.location.start + last.location.length, 1 },
        .message=std::move(message),
    });
}

auto Scanner::peek() -> char
{
    if (isAtEnd()) return '\0';
    return source.at(current);
}

auto Scanner::consume() -> char
{
    return source.at(current++);
}

bool Scanner::match(char c)
{
    if (isAtEnd()) return false;
    if (peek() != c) return false;

    consume();
    return true;
}

bool Scanner::isAlpha(char c)
{
    return (c >= 'a' && c <= 'z')
        || (c >= 'A' && c <= 'Z')
        || c == '_';
}

bool Scanner::isDigit(char c)
{
    return c >= '0' && c <= '9';
}

bool Scanner::isAlphaNumeric(char c)
{
    return isAlpha(c) || isDigit(c);
}

void Scanner::addToken(TokenType type)
{
    addToken(type, std::monostate{});
}

void Scanner::addToken(TokenType type, Token::Value value)
{
    const size_t len = current - start;

    tokens.emplace_back(Token{
        .type = type,
        .lexeme = source.substr(start, len),
        .value = std::move(value),
        .location{
            .line = line,
            .start = character,
            .length = len,
        }
    });

    if (type == TokenType::eNewline) character = 1;
    else character += len;
}
