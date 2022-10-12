#pragma once

#include <cstdint>
#include <istream>
#include <string>
#include <unordered_map>
#include <vector>

#include <trc_util/Exception.h>

namespace shader_edit
{
    class SyntaxError : public trc::Exception
    {
    public:
        SyntaxError(uint32_t line, const std::string& error);

        /** @return uint32_t The line in which the syntax error occured */
        auto getLine() const -> uint32_t;

    private:
        uint32_t line;
    };

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

        uint32_t line{ UINT32_MAX };
        size_t firstChar{ 0 };
        size_t lastChar{ std::string::npos };
    };

    struct ParseResult
    {
        std::vector<std::string> lines;

        std::unordered_map<std::string, ParsedVariable> variablesByName{};
    };

    /**
     * @throw SyntaxError if a syntax error is encountered
     */
    auto parseShader(std::istream& is) -> ParseResult;

    /**
     * @throw SyntaxError if a syntax error is encountered
     */
    auto parseShader(std::vector<std::string> lines) -> ParseResult;
} // namespace shader_edit
