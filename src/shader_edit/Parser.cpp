#include "shader_edit/Parser.h"

#include <optional>
#include <iostream>

#include "shader_edit/StringUtil.h"



namespace shader_edit
{

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



auto parseVariable(const std::string& line) -> std::optional<Variable>
{
    constexpr size_t NAME_POS{ 1 };

    auto split = splitString(line, " ");
    removeEmpty(split);

    // Test if line is a variable declaration
    const bool isVar = split.size() >= 1 && split.at(0) == VAR_DECL;
    if (!isVar) return std::nullopt;

    // Parse name
    if (split.size() < 2) {
        throw SyntaxError("Expected variable name, found none");
    }
    auto& name = split.at(NAME_POS);

    return Variable{ .name=std::move(name) };
}



auto parse(std::istream& is) -> ParseResult
{
    return parse(toLines(is));
}

auto parse(std::vector<std::string> _lines) -> ParseResult
{
    ParseResult result{ .lines=std::move(_lines) };

    for (uint i = 0; i < result.lines.size(); i++)
    {
        auto& line = result.lines.at(i);
        std::replace(line.begin(), line.end(), '\t', ' ');

        try {
            auto var = parseVariable(line);
            if (var.has_value())
            {
                result.variablesByLine[i] = var.value();
                result.variablesByName[var.value().name] = i;
            }
        }
        catch (const SyntaxError& err) {
            std::cout << "[Syntax error in line " << i << "]: " << err.what() << "\n";
        }
    }

    return result;
}

} // namespace shader_edit
