#include "shader_tools/ShaderDocumentParser.h"

#include <algorithm>
#include <optional>

#include <trc_util/StringManip.h>



namespace shader_edit
{

using trc::util::splitString;
using trc::util::removeEmpty;
using trc::util::readLines;

constexpr auto VAR_DECL{ "//$" };
constexpr auto INLINE_VAR_DECL{ '$' };



SyntaxError::SyntaxError(uint line, const std::string& error)
    :
    trc::Exception("[Syntax error in line " + std::to_string(line) + "]: " + error),
    line(line)
{
}



auto parseVariable(const std::string& line) -> std::optional<ParsedVariable>
{
    constexpr size_t NAME_POS{ 1 };

    auto split = splitString(line, " ");
    removeEmpty(split);

    if (split.empty()) return std::nullopt;

    // Test if line is a variable declaration
    if (split.at(0) == VAR_DECL)
    {
        // Parse name
        if (split.size() < 2) {
            throw std::runtime_error("Expected variable name, found none");
        }

        return ParsedVariable{ .name=split.at(NAME_POS) };
    }

    // Test if line contains inline variable
    for (size_t first = 0; const char c : line)
    {
        if (c == INLINE_VAR_DECL)
        {
            ++first;
            size_t last{ line.size() };
            for (size_t i = first; i < last; ++i)
            {
                const char c = line[i];
                const bool alpha   = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
                const bool num     = c >= '0' && c <= '9';
                const bool special = c == '_';
                if (!(alpha || num || special))
                {
                    last = i;
                    break;
                }
            }

            const std::string varName = line.substr(first, last - first);
            if (varName.empty()) {
                throw std::runtime_error("Expected variable name, found none");
            }

            return ParsedVariable{ .name=varName, .firstChar=first - 1, .lastChar=last };
        }
        ++first;
    }

    return std::nullopt;
}



auto parseShader(std::istream& is) -> ParseResult
{
    return parseShader(readLines(is));
}

auto parseShader(std::vector<std::string> _lines) -> ParseResult
{
    ParseResult result{ .lines=std::move(_lines) };

    for (uint i = 0; auto& line : result.lines)
    {
        // We want to split by spaces: Treat all tabs like spaces
        std::replace(line.begin(), line.end(), '\t', ' ');

        try {
            auto var = parseVariable(line);
            if (var.has_value())
            {
                var->line = i;
                result.variablesByName.try_emplace(var->name, std::move(var.value()));
            }
        }
        catch (const std::runtime_error& err) {
            throw SyntaxError(i, err.what());
        }
        ++i;
    }

    return result;
}

} // namespace shader_edit
