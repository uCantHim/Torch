#include "parser.h"

#include <cassert>

#include <format>
#include <istream>
#include <iostream>
#include <ranges>
#include <string_view>

#include <trc_util/StringManip.h>



namespace cloth::parser
{

constexpr std::string_view kVarDeclStart = "$";
constexpr std::string_view kNamespaceSep = ":";
constexpr std::string_view kCommentStart = "//";
constexpr std::string_view kArgumentListStart = "[";
constexpr std::string_view kArgumentListEnd = "]";

class Lexer
{
public:
    explicit Lexer(std::string_view str)
        : curPos(0), str(str) {}

    auto pos() const -> size_t {
        return curPos;
    }

    bool eof() const {
        return curPos == str.size();
    }

    auto peek() -> char
    {
        assert(!eof());
        return str[curPos];
    }

    auto consume() -> char
    {
        assert(!eof());
        return str[curPos++];
    }

    bool peek(std::string_view pattern) const
    {
        return str.substr(pos()).starts_with(pattern);
    }

    bool tryConsume(std::string_view pattern)
    {
        if (peek(pattern))
        {
            curPos += pattern.size();
            return true;
        }
        return false;
    }

    auto consumeWhile(auto&& func) -> std::string_view
    {
        const auto begin = pos();
        while (!eof() && func(peek())) {
            consume();
        }
        return str.substr(begin, pos() - begin);
    }

    auto consumeUntil(std::string_view pattern) -> std::optional<std::string_view>
    {
        const auto begin = pos();
        while (!eof() && !str.substr(pos()).starts_with(pattern)) {
            consume();
        }
        if (eof()) {
            return std::nullopt;
        }
        return str.substr(begin, pos() - begin);
    }

    auto consumeUntil(auto&& func) -> std::string_view
    {
        const auto begin = pos();
        while (!eof() && !func(peek())) {
            consume();
        }
        return str.substr(begin, pos() - begin);
    }

private:
    size_t curPos{ 0 };
    std::string_view str;
};

class Parser
{
public:
    Parser() = default;

    void parse(std::vector<std::string> document)
    {
        for (auto [i, line] : std::views::enumerate(document))
        {
            currentLine = i;
            parseLine(line);
            lines.emplace_back(std::move(line));
        }
    }

    auto makeResult() -> std::expected<Result, IncompleteResult>
    {
        Result res{
            .lines=std::move(lines),
            .variablesByName=std::move(variablesByName),
            .variablesInOrderOfOccurrence=std::move(variablesInOrderOfOccurrence),
        };

        if (errors.empty()) {
            return res;
        }
        return std::unexpected(IncompleteResult{
            .errors=std::move(errors),
            .partialResult=std::move(res),
        });
    }

private:
    void parseLine(std::string_view line)
    {
        Lexer lex{ line };
        while (!lex.eof())
        {
            if (lex.peek(kCommentStart)) {
                break;
            }

            if (lex.peek(kVarDeclStart)) {
                parseVariable(lex);
            }
            else {
                lex.consume();
            }
        }
    }

    void parseVariable(Lexer& lex)
    {
        Variable var;
        var.location = { .line=currentLine, .firstChar=lex.pos(), .endChar=lex.pos(), };

        // Consume the initial '$' - we still want to include it in the
        // identifier's character range, though.
        [[maybe_unused]]
        const bool _start = lex.tryConsume(kVarDeclStart);
        assert(_start);

        // Parse identifier
        auto id = parseIdentifier(lex);
        if (id.empty())
        {
            emitError(Error{
                .location{ .line=currentLine, .firstChar=var.location.firstChar, .endChar=lex.pos(), },
                .message="Expected an identifier."
            });
            return;
        }
        var.id = FullId::fromString(id, kNamespaceSep);

        // Parse argument list
        if (auto args = parseArgumentList(lex))
        {
            if (args->has_value()) {
                var.arguments = **args;
            }
            else {
                emitError(std::move(args->error()));
            }
        }

        var.location.endChar = lex.pos();
        emitVariable(std::move(var));
    }

    auto parseIdentifier(Lexer& lex) const -> std::string_view
    {
        return lex.consumeWhile([](char c) -> bool {
            const bool alpha    = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
            const bool num      = c >= '0' && c <= '9';
            const bool wordchar = c == '_' || c == ':';
            return alpha || num || wordchar;
        });
    }

    auto parseArgumentList(Lexer& lex)
        -> std::optional<std::expected<std::vector<Argument>, Error>>
    {
        if (!lex.tryConsume(kArgumentListStart)) {
            return std::nullopt;
        }

        auto content = lex.consumeUntil(kArgumentListEnd);
        if (!content)
        {
            return std::unexpected(Error{
                .location{ .line=currentLine, .firstChar=lex.pos() - 1, .endChar=lex.pos() },
                .message=std::format("Expected symbol {}, got EOF.", kArgumentListEnd)
            });
        }
        lex.consume();  // Consume the closing ']'

        auto trimWhitespace = [](std::string_view str) {
            size_t begin{ 0 };
            size_t end{ str.size() };
            while (begin < str.size() && std::isspace(str[begin])) ++begin;
            while (end > begin && std::isspace(str[end-1])) --end;

            assert(end >= begin);
            return str.substr(begin, end - begin);
        };

        // NOTE: I need to parse this manually if string arguments can contain commas
        auto args = *content
            | std::views::split(',')
            | std::views::transform([](auto s){ return std::string_view{ s }; })
            | std::views::transform(trimWhitespace)
            | std::views::transform([this](auto sv) -> Argument {
                if (auto arg = parseArgument(sv)) {
                    return *arg;
                }
                return { .type=Argument::Type::eExternalExpression, .content="" };
            });

        return std::ranges::to<std::vector>(args);
    }

    auto parseArgument(std::string_view str) -> std::expected<Argument, Error>
    {
        // Emit a variable if the argument is a variable reference
        // ...

        return Argument{
            .type=Argument::Type::eExternalExpression,
            .content{ str },
        };
    }

    void emitError(Error&& err)
    {
        errors.emplace_back(std::move(err));
    }

    void emitVariable(Variable&& var)
    {
        variablesInOrderOfOccurrence.emplace_back(var);
        auto [it, _] = variablesByName.try_emplace(var.id);
        it->second.emplace_back(var);
    }

    ui32 currentLine{ 0 };
    std::vector<std::string> lines;

    std::vector<Error> errors;
    std::unordered_map<FullId, std::vector<Variable>> variablesByName;
    std::vector<Variable> variablesInOrderOfOccurrence;
};

auto Result::toDocument() const -> shader_edit::ShaderDocument
{
    auto convert = [](const Variable& var) -> shader_edit::Variable
    {
        const auto loc = var.location;
        return shader_edit::Variable{
            .name=var.id.id,
            .location{ .line=loc.line, .firstChar=loc.firstChar, .endChar=loc.endChar, },
        };
    };

    shader_edit::ParseResult res{ .lines=lines, };
    for (const auto& [id, vars] : variablesByName)
    {
        auto [it, _] = res.variablesByName.try_emplace(id.id);
        for (const auto& var : vars) {
            it->second.emplace_back(convert(var));
        }
    }
    for (const auto& var : variablesInOrderOfOccurrence)
    {
        auto _var = convert(var);

        auto [it, _] = res.variablesByName.try_emplace(_var.name);
        it->second.emplace_back(_var);
        res.variablesInOrderOfOccurrence.emplace_back(std::move(_var));
    }

    return shader_edit::ShaderDocument{ std::move(res) };
}

auto parseDocument(std::istream& is) -> std::expected<Result, IncompleteResult>
{
    return parseDocument(trc::util::readLines(is));
}

auto parseDocument(std::vector<std::string> lines) -> std::expected<Result, IncompleteResult>
{
    Parser parser;
    parser.parse(std::move(lines));
    return parser.makeResult();
}

} // namespace cloth
