#include "shader_tools/ShaderDocumentParser.h"

#include <optional>
#include <algorithm>
#include <iostream>

#include <trc_util/StringManip.h>



namespace shader_edit
{

using trc::util::splitString;
using trc::util::removeEmpty;
using trc::util::readLines;

constexpr auto VAR_DECL{ "//$" };
constexpr auto INLINE_VAR_DECL{ "$" };



class SyntaxError : public std::exception
{
public:
    SyntaxError(std::string error) : str(std::move(error)) {}

    auto what() const noexcept -> const char* override {
        return str.c_str();
    }

private:
    std::string str;
};



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
            throw SyntaxError("Expected variable name, found none");
        }

        return ParsedVariable{ .name=split.at(NAME_POS) };
    }

    // Test if line contains inline variable
    for (const auto& chunk : split)
    {
        const bool isInlineVar = !chunk.empty() && chunk.starts_with(INLINE_VAR_DECL);
        if (isInlineVar)
        {
            const std::string varName = chunk.substr(1);
            if (varName.empty()) {
                throw SyntaxError("Expected variable name, found none");
            }

            const size_t first = line.find(chunk);
            return ParsedVariable{ .name=varName, .firstChar=first, .lastChar=first + chunk.size() };
        }
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
        catch (const SyntaxError& err)
        {
            std::stringstream ss;
            ss << "[Syntax error in line " << i << "]: " << err.what();
            throw SyntaxError(ss.str());
        }
        ++i;
    }

    return result;
}

} // namespace shader_edit
