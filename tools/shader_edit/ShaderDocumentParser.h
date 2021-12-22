#pragma once

#include <string>
#include <vector>
#include <unordered_map>

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
    struct Variable
    {
        std::string name;
    };

    struct ParseResult
    {
        std::vector<std::string> lines;

        std::unordered_map<uint, Variable> variablesByLine{};
        std::unordered_map<std::string, uint> variablesByName{};
    };

    constexpr auto VAR_DECL{ "//$" };

    auto parseShader(std::istream& is) -> ParseResult;
    auto parseShader(std::vector<std::string> lines) -> ParseResult;
} // namespace shader_edit
