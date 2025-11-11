#pragma once

#include <any>
#include <expected>
#include <generator>
#include <iosfwd>
#include <ranges>
#include <string>
#include <unordered_map>
#include <vector>

#include <shader_tools/ShaderDocument.h>
#include <trc/Types.h>

#include "full_id.h"
#include "parse_utils.h"

namespace vk {
    enum class ShaderStageFlagBits : unsigned int;
}

namespace cloth::parser
{
    using namespace trc::basic_types;

    struct Location
    {
        ui32 line{ UINT32_MAX };
        size_t firstChar{ 0 };
        size_t endChar{ std::string::npos };

        bool operator==(const Location&) const = default;
        bool operator<(const Location& other) const {
            return line < other.line || (line == other.line && firstChar < other.firstChar);
        }

        auto operator<=>(const Location&) const = default;
    };
}

template<>
struct std::hash<cloth::parser::Location>
{
    auto operator()(const cloth::parser::Location& loc) const -> size_t {
        return std::hash<uint32_t>{}(loc.line << 16 | loc.firstChar << 8 | loc.endChar);
    }
};

namespace cloth::parser
{
    struct Error
    {
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
        Location location;

        /**
         * A lazy solution.
         *
         * String:     [std::string] The string's content, without enclosing quotes.
         * Expression: [std::string] The expression code as a string.
         * Variable:   [parser::Variable] The variable.
         */
        std::any content;
    };

    struct Variable
    {
        FullId id;
        std::vector<Argument> arguments;

        Location location;

        /**
         * Unmodified Cloth code that declares the variable access.
         *
         * The parser and the code generator work in terms of unique values
         * that may be used at several locations throughout the document.
         *
         * Since 'variables' (more correctly thought of as built-in
         * capabilities or 'builtins') can have arguments and thus are more
         * like functions, the name of a variable is not sufficient anymore to
         * distinguish between unique values: `$vertexNormal` may be a constant
         * expression, but `$texture["/images/stone.ta"]` and
         * `$texture["/images/wood.ta"]` yield different values despite the
         * identifiers being "texture". Thus, we store the full declaration
         * text and use it to determine uniqueness of value.
         */
        std::string fullDeclText;
    };

    struct Result
    {
        std::vector<std::string> lines;

        /**
         * The respective first occurrence of every unique variable in the document.
         */
        std::vector<Variable> variablesInOrderOfOccurrence;

        /**
         * Maps [<first-occurence> -> <additional-occurrences>]
         *
         * Two variable references with the same full declaration text (e.g.
         * '$texture["/my/image.png"]') always have the same unique value. This map
         * points from unique values - in this described sense - to their points of
         * reference in the document.
         */
        std::unordered_map<std::string, std::vector<Location>> allReferences;
    };

    struct IncompleteResult
    {
        std::vector<Error> errors;
        Result partialResult;
    };

    auto parseDocument(std::istream& is) -> std::expected<Result, IncompleteResult>;
    auto parseDocument(std::vector<std::string> lines) -> std::expected<Result, IncompleteResult>;

    enum class ShaderStage
    {
        eVertex,
        eGeometry,
        eFragment,
    };

    auto to_string(ShaderStage stage) -> std::string_view;
    auto toVulkanEnum(const cloth::parser::ShaderStage stage) -> vk::ShaderStageFlagBits;
    auto fromVulkanEnum(const vk::ShaderStageFlagBits& stage) -> ShaderStage;

    struct MultiDocumentResult
    {
        std::vector<std::string> originalLines;

        // Code shared among all shader stages. This is code that does not
        // reside in any block.
        //
        // Note: This is extremely WIP. The shared code is currently collected
        // correctly, but not inserted anywhere because it causes too many
        // problems with line numbering, variable declaration code placement,
        // different implementations/availability of builtins in different
        // shader stages, ...
        std::vector<std::string> sharedCode;

        // Shader stages that compiled successfully.
        std::unordered_map<ShaderStage, Result> shaderStages;

        // Shader stages that did not compile successfully.
        std::unordered_map<ShaderStage, IncompleteResult> incompleteShaderStages;

        // Locations of shader module blocks in the original document, excluding
        // the block declarations.
        //
        // Only contains a shader stage if the entire block declaration was
        // parsed successfully.
        std::unordered_map<ShaderStage, util::BlockLocation> shaderStageLocations;

        // Only errors regarding parsing of block declarations.
        std::vector<Error> errors;

        /**
         * Iterate over all parse error, meaning global document-level errors
         * and shader-block internal errors.
         */
        auto allErrors() const -> std::generator<const Error&>
        {
            co_yield std::ranges::elements_of(errors);
            for (auto& [_, result] : incompleteShaderStages) {
                co_yield std::ranges::elements_of(result.errors);
            }
        }
    };

    auto parseMultiDocument(const std::vector<std::string>& lines) -> MultiDocumentResult;
} // namespace cloth::parser
