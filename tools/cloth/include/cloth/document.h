#pragma once

#include <expected>
#include <generator>
#include <string>

#include "parser.h"
#include "types.h"

namespace cloth
{
    struct CompileError
    {
        std::string message;
    };

    /**
     * @brief A parsed Cloth document.
     */
    class Document
    {
    public:
        Document() = default;

        explicit Document(parser::Result parseResult);

        auto allVariables() -> std::generator<const FullId&>;
        auto unsetVariables() -> std::generator<const FullId&>;

        auto findOccurrences(const FullId& varName)
            -> std::generator<parser::Location>;

        /**
         * @brief Set the value of a variable
         */
        void set(const FullId& name, std::string value);

        /**
         * @brief Compile variable settings into a document.
         *
         * @param bool allowUnsetVariables If false, return an error if
         *        the document has one or more variables for which no value
         *        has been set.
         *
         * @return A text document on success, or an error on failure.
         */
        auto compile(bool allowUnsetVariables = false) const
            -> std::expected<std::string, CompileError>;

        auto getLine(size_t idx) -> const std::string*;
        auto getLines() -> const std::vector<std::string>&;

    private:
        /** Stores the entire document and all variables discovered during parsing. */
        parser::Result parseData;

        /** The underlying document. I don't want to duplicate the implementations. */
        shader_edit::ShaderDocument doc;
    };
} // namespace cloth
