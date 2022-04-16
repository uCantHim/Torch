#include "Scanner.h"

#include "ErrorReporter.h"



const std::unordered_map<std::string, TokenType> Scanner::keywords{
    { "match", TokenType::eMatch },
};



Scanner::Scanner(std::string source, ErrorReporter& errorReporter)
    :
    source(std::move(source)),
    errorReporter(&errorReporter)
{
}

auto Scanner::scanTokens() -> std::vector<Token>
{
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
    case '{': addToken(TokenType::eLeftBrace); break;
    case '}': addToken(TokenType::eRightBrace); break;
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
        break;
    case ' ':
    case '\t':
    case '\r':
        break;
    case '\n':
        ++line;
        addToken(TokenType::eNewline);
        start = current;
        scanIndent();
        break;
    default:
        if (isAlpha(c)) {
            scanIdentifier();
        }
        else {
            error("Unexpected character: '" + std::string(&c) + "'");
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
    std::string val = source.substr(start, len);
    addToken(TokenType::eLiteralString, std::move(val));
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

    if (spaces == 0) return;
    if (spaces % 4 == 0) {
        addToken(TokenType::eIndent, Token::IndentLevel(spaces % 4));
    }
    else {
        error("Indent must be exactly four spaces per indent level.");
    }
}

void Scanner::error(std::string message)
{
    errorReporter->error({
        .location={ line, 0, 1 },
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
            .start = 0,
            .length = len,
        }
    });
}
