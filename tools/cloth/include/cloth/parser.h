#pragma once

#include <expected>
#include <iosfwd>
#include <string>
#include <unordered_map>
#include <vector>

#include <shader_tools/ShaderDocument.h>
#include <trc/Types.h>

#include "types.h"

namespace cloth::parser
{
    using namespace trc::basic_types;

    struct Location
    {
        ui32 line{ UINT32_MAX };
        size_t firstChar{ 0 };
        size_t endChar{ std::string::npos };
    };

    struct Error
    {
        enum class Code
        {
            eSyntaxError,
            eTypeError,
        };

        Code code;
        Location location;
        std::string message;
    };

    struct Argument
    {
        enum class Type
        {
            eString,
            eExternalExpression,
            eVariable,
        };

        Type type;

        /**
         * String:     The string's content, without enclosing quotes.
         * Expression: The expression code as a string.
         * Variable:   The variable's full ID
         */
        std::string content;
    };

    struct Variable
    {
        std::string id;
        FullId fullId;
        std::vector<std::string> namespaces;

        std::vector<Argument> arguments;

        Location location;
    };

    /**
     * @brief
     */
    struct Result
    {
        std::vector<std::string> lines;

        std::unordered_map<FullId, std::vector<Variable>> variablesByName;
        std::vector<Variable> variablesInOrderOfOccurrence;

        auto toDocument() const -> shader_edit::ShaderDocument;
    };

    struct IncompleteResult
    {
        std::vector<Error> errors;
        Result partialResult;
    };

    auto parseClothDocument(std::istream& is)
        -> std::expected<Result, IncompleteResult>;
} // namespace cloth
