#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <istream>

namespace shader_edit
{
    /**
     * Shader variable syntax:
     *
     *     //$ name
     *
     * The sequence '//$' at the beginning of a line designates a variable
     * definition.
     *
     * name: A name by which the variable can be referenced
     */
    struct ParsedVariable
    {
        std::string name;

        uint line{ UINT32_MAX };
        size_t firstChar{ 0 };
        size_t lastChar{ std::string::npos };
    };

    struct ParseResult
    {
        std::vector<std::string> lines;

        std::unordered_map<std::string, ParsedVariable> variablesByName{};
    };

    auto parseShader(std::istream& is) -> ParseResult;
    auto parseShader(std::vector<std::string> lines) -> ParseResult;
} // namespace shader_edit
