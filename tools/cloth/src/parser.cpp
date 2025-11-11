#include "parser.h"

#include <cassert>

#include <format>
#include <istream>
#include <iostream>
#include <ranges>
#include <string_view>

#include <trc_util/StringManip.h>
#include <trc/VulkanInclude.h>

#include "parse_utils.h"



namespace cloth::parser
{

constexpr std::string_view kVarDeclStart = "$";
constexpr std::string_view kNamespaceSep = ":";
constexpr std::string_view kCommentStart = "//";
constexpr std::string_view kArgumentListStart = "[";
constexpr std::string_view kArgumentListEnd = "]";
constexpr std::string_view kArgumentSep = ",";

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

    void skipWhitespace()
    {
        while (!eof() && std::isspace(peek())) {
            consume();
        }
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

    bool peek(char c) const
    {
        return str.substr(pos()).starts_with(c);
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

    auto consumeWhile(std::invocable<char> auto&& func) -> std::string_view
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

    auto consumeUntil(std::invocable<char> auto&& func) -> std::string_view
    {
        const auto begin = pos();
        while (!eof() && !func(peek())) {
            consume();
        }
        return str.substr(begin, pos() - begin);
    }

    /** Return the entire remaining string. */
    auto consumeAll() -> std::string_view
    {
        auto res = str.substr(curPos);
        curPos = str.size();
        return res;
    }

    /**
     * Access a range of text from the underlying buffer, ignoring the current
     * position.
     */
    auto getAbsoluteRange(size_t begin, size_t end) -> std::string_view
    {
        assert(begin < str.size() && end <= str.size());
        return str.substr(begin, end - begin);
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
            .variablesInOrderOfOccurrence=std::move(variablesInOrderOfOccurrence),
            .allReferences=std::move(allReferences),
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
        auto processVariable = [this](auto&& expectedVar) {
            if (expectedVar) {
                emitVariable(std::move(expectedVar.value()));
            }
            else {
                emitError(std::move(expectedVar.error()));
            }
        };

        Lexer lex{ line };
        while (!lex.eof())
        {
            if (lex.peek(kCommentStart)) {
                break;
            }

            if (lex.peek(kVarDeclStart)) {
                processVariable(parseVariable(lex));
            }
            else {
                lex.consume();
            }
        }
    }

    auto parseVariable(Lexer& lex) -> std::expected<Variable, Error>
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
            return std::unexpected(Error{
                .location{ .line=currentLine, .firstChar=var.location.firstChar, .endChar=lex.pos(), },
                .message="Expected an identifier."
            });
        }
        var.id = FullId::fromString(id, kNamespaceSep);

        // Parse argument list
        if (auto args = parseArgumentList(lex))
        {
            if (args->has_value()) {
                var.arguments = **args;
            }
            else {
                return std::unexpected(args->error());
            }
        }

        var.location.endChar = lex.pos();
        var.fullDeclText = lex.getAbsoluteRange(var.location.firstChar, lex.pos());
        return var;
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

        std::vector<Argument> args;
        while (!lex.eof() && !lex.peek(kArgumentListEnd))
        {
            lex.skipWhitespace();  // Whitespace before the argument

            auto arg = parseArgument(lex);
            if (arg) {
                args.emplace_back(std::move(*arg));
            }
            else {
                return std::unexpected(arg.error());
            }

            lex.skipWhitespace();  // Whitespace after the argument
            if (!lex.tryConsume(kArgumentSep)) {
                break;
            }
        }

        lex.skipWhitespace();
        if (!lex.tryConsume(kArgumentListEnd))
        {
            return std::unexpected(Error{
                .location{ currentLine, lex.pos(), lex.pos() + 1 },
                .message=std::format("Expected {}.", kArgumentListEnd)
            });
        }
        return args;
    }

    auto parseArgument(Lexer& lex) -> std::expected<Argument, Error>
    {
        // Try to parse a Cloth variable
        if (lex.peek(kVarDeclStart))
        {
            auto var = parseVariable(lex);
            if (var) {
                Argument arg{
                    .type=Argument::Type::eVariable,
                    .location=var->location,
                    .content=var.value()
                };
                emitVariable(std::move(*var));
                return arg;
            }
            return std::unexpected(var.error());
        }
        // Try to parse a string literal
        else if (lex.tryConsume("\""))
        {
            const auto begin = lex.pos();
            const auto arg = lex.consumeUntil("\"");
            if (arg) {
                lex.consume();
                return Argument{
                    .type=Argument::Type::eString,
                    .location{ .line=currentLine, .firstChar=begin, .endChar=lex.pos() },
                    .content=std::string{ *arg }
                };
            }
            return std::unexpected(Error{
                Location{ currentLine, lex.pos() - 1, lex.pos() },
                "Expected closing quote '\"'."
            });
        }

        // Argument is not a Cloth value type - try to interpret it as an
        // external GLSL expression.
        const auto begin = lex.pos();
        auto isArgumentEnd = [](char c){ return c == ']' || c == ','; };
        auto expr = lex.consumeUntil(isArgumentEnd);
        return Argument{
            .type=Argument::Type::eExternalExpression,
            .location{ .line=currentLine, .firstChar=begin, .endChar=lex.pos() },
            .content=std::string{ expr },
        };
    }

    void emitError(Error&& err)
    {
        errors.emplace_back(std::move(err));
    }

    void emitVariable(Variable&& var)
    {
        auto [it, success] = allReferences.try_emplace(var.fullDeclText);
        it->second.emplace_back(var.location);
        if (success) {
            // Only add the first occurrence of any unique variable
            variablesInOrderOfOccurrence.emplace_back(std::move(var));
        }
    }

    ui32 currentLine{ 0 };
    std::vector<std::string> lines;

    std::vector<Error> errors;

    /**
     * The respective first occurrence of every unique variable in the document.
     */
    std::vector<Variable> variablesInOrderOfOccurrence;

    /**
     * Maps [<decl-text> -> <occurrences>]
     *
     * Two variable references with the same full declaration text (e.g.
     * '$texture["/my/image.png"]') always have the same unique value. This map
     * points from unique values - in this described sense - to their points of
     * reference in the document.
     */
    std::unordered_map<std::string, std::vector<Location>> allReferences;
};

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

auto to_string(ShaderStage stage) -> std::string_view
{
    switch (stage)
    {
    case ShaderStage::eVertex: return "Vertex";
    case ShaderStage::eGeometry: return "Geometry";
    case ShaderStage::eFragment: return "Fragment";
    }

    std::unreachable();
}

auto toVulkanEnum(const cloth::parser::ShaderStage stage) -> vk::ShaderStageFlagBits
{
    switch (stage)
    {
    case ShaderStage::eFragment:
        return vk::ShaderStageFlagBits::eFragment;
    case ShaderStage::eVertex:
        return vk::ShaderStageFlagBits::eVertex;
    case ShaderStage::eGeometry:
        return vk::ShaderStageFlagBits::eGeometry;
    }
    std::unreachable();
};

auto fromVulkanEnum(const vk::ShaderStageFlagBits& stage) -> ShaderStage
{
    switch (stage) {
    case vk::ShaderStageFlagBits::eFragment:
        return ShaderStage::eFragment;
    case vk::ShaderStageFlagBits::eVertex:
        return ShaderStage::eVertex;
    case vk::ShaderStageFlagBits::eGeometry:
        return ShaderStage::eGeometry;
    default:
        throw std::invalid_argument(std::format("Shader stage {} is not valid in Cloth.",
                                                vk::to_string(stage)));
    }
    std::unreachable();
};

/**
 * @return The result only has the members `originalLines`, `sharedCode`,
 *         `shaderStageLocations`, and `errors` populated.
 */
auto findBlocks(const std::vector<std::string>& lines) -> MultiDocumentResult
{
    constexpr auto kBlockNameDeclSymbol = '@';
    constexpr auto kBlockOpeningSymbol = '{';
    constexpr auto isIdCharacter = [](char c){ return std::isalpha(c) || c == '_'; };

    constexpr auto idToShaderStage = [](std::string_view str) -> std::optional<ShaderStage> {
        if (str == "vertex") {
            return ShaderStage::eVertex;
        }
        if (str == "geometry") {
            return ShaderStage::eGeometry;
        }
        if (str == "fragment") {
            return ShaderStage::eFragment;
        }
        return std::nullopt;
    };

    MultiDocumentResult res;
    res.originalLines = lines;

    auto sharedCodeBeginLine = lines.begin();
    for (auto lineIt = lines.begin(); lineIt != lines.end(); ++lineIt)
    {
        const auto& line = *lineIt;
        const size_t i = lineIt - lines.begin();

        // Find a shader block declaration (starts with '@')
        auto pos = line.find(kBlockNameDeclSymbol);
        if (pos == std::string::npos) {
            continue;
        }
        if (pos != 0)
        {
            // Error: Expected block declaration to be at the start of a line.
            res.errors.emplace_back(
                Location{ ui32(i), pos, pos + 1 },
                "Block declarations must be placed at the beginning of a line."
            );
            continue;
        }

        // Parse the shader block name
        Lexer lexer{ std::string_view{ line.begin() + pos, line.end() } };
        lexer.consume();  // Consume the initial '@' symbol
        const auto blockName = lexer.consumeWhile(isIdCharacter);
        if (blockName.empty())
        {
            // Error: Missing block name identifier.
            res.errors.emplace_back(
                Location{ ui32(i), pos, lexer.pos() + 1 },
                "Expected an identifier."
            );
            continue;
        }

        // Validate the shader block name
        const auto shaderStage = idToShaderStage(blockName);
        if (!shaderStage)
        {
            // Error: Invalid block name.
            res.errors.emplace_back(
                Location{ ui32(i), pos, lexer.pos() },
                std::format("\"{}\" is not a valid shader stage.", blockName)
            );
            continue;
        }

        // Find the opening brace '{'
        lexer.skipWhitespace();
        if (!lexer.peek(kBlockOpeningSymbol))
        {
            // Error: Expected '{'.
            res.errors.emplace_back(
                Location{ ui32(i), lexer.pos(), lexer.pos() + 1 },
                "Expected '{'."
            );
            continue;
        }
        lexer.consume();

        // Forbid putting code on the same line with the block decl
        lexer.skipWhitespace();
        if (lexer.pos() != line.size())
        {
            res.errors.emplace_back(
                Location{ ui32(i), lexer.pos(), line.size() },
                "Don't put code on the same line with a block declaration."
            );
            continue;
        }

        // Find the closing brace '}'
        const auto close = util::findClosingBrace(lines, { i, lexer.pos() });
        if (!close)
        {
            // Error: Expected closing '}'
            res.errors.emplace_back(
                Location{ ui32(lines.size() - 1), lines.back().size() - 1, lines.back().size() },
                "Expected closing '}'."
            );
            continue;
        }

        // Forbid putting something in front of the closing brace.
        if (close->pos != 0)
        {
            res.errors.emplace_back(
                Location{ ui32(close->line), 0, close->pos },
                "You shall put the block closing brace on a separate line."
            );
            continue;
        }

        // The block is complete - emit it.
        const auto blockEnd = lines.begin() + close->line;
        res.shaderStageLocations.try_emplace(
            *shaderStage,
            util::BlockLocation{
                .declBegin{ i, pos },
                .declEnd{ close->line, close->pos },
                .bodyBegin{ i + 1, 0 },
                .bodyEnd{ close->line - 1, lines[close->line - 1].size() },
            }
        );

        // Store shared code (code outside of the block)
        res.sharedCode.insert(res.sharedCode.end(), sharedCodeBeginLine, lineIt);
        sharedCodeBeginLine = blockEnd + 1;

        lineIt = blockEnd;  // Will be incremented by the loop
    }

    return res;
}

auto parseMultiDocument(const std::vector<std::string>& lines) -> MultiDocumentResult
{
    MultiDocumentResult res = findBlocks(lines);

    // Parse blocks as standalone shader modules
    for (const auto& [stage, blockLocation] : res.shaderStageLocations)
    {
        // TODO:
        // Prepend shared code to each module individually.
        // This is necessary because variables in the shared code may get
        // resolved differently depending on the shader stage.
        //
        // This introduces many problems with line numbering and so forth...

        std::vector<std::string> blockLines{
            lines.begin() + blockLocation.bodyBegin.line,
            lines.begin() + blockLocation.bodyEnd.line
        };

        auto block = parseDocument(blockLines);
        if (block) {
            res.shaderStages.try_emplace(stage, std::move(*block));
        }
        else {
            res.incompleteShaderStages.try_emplace(stage, std::move(block.error()));
        }
    }

    // Make error locations relative to the original document lines.
    for (auto& [stage, errs] : res.incompleteShaderStages)
    {
        const auto& loc = res.shaderStageLocations.at(stage);
        for (auto& err : errs.errors) {
            err.location.line += loc.bodyBegin.line;
        }
    }

    return res;
}

} // namespace cloth
