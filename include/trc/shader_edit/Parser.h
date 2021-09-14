#pragma once

#include <string>
#include <vector>
#include <unordered_map>

namespace shader_edit
{
    /**
     * Shader variable syntax:
     *
     *     //$ type name
     *
     * The sequence '//$' at the beginning of a line designates a variable
     * definition.
     *
     * type: The variable's type declaration
     * name: A name by which the variable can be referenced
     */
    struct Variable
    {
        enum class Type
        {
            eLayoutQualifier,

            eCustom,
        };

        Type type;
        std::string name;
    };

    struct ParseResult
    {
        std::vector<std::string> lines;

        std::unordered_map<uint, Variable> variablesByLine{};
        std::unordered_map<std::string, uint> variablesByName{};
    };

    constexpr auto VAR_DECL{ "//$" };

    constexpr auto TYPE_NAME_LAYOUT_QUALIFIER{ "layout" };
    constexpr auto TYPE_NAME_CUSTOM{ "custom" };

    auto parse(std::istream& is) -> ParseResult;
    auto parse(std::vector<std::string> lines) -> ParseResult;
} // namespace shader_edit
