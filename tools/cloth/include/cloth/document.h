#pragma once

#include <generator>
#include <string>
#include <unordered_map>
#include <vector>

#include "parser.h"

namespace cloth
{
    struct DocumentError
    {
        std::string message;
    };

    /**
     * @brief A Cloth document.
     *
     * Handles variable replacement and provides some information about
     * variables in the document.
     */
    class Document
    {
    public:
        Document() = default;

        explicit Document(parser::Result parseResult);

        auto allVariables() const -> std::generator<const parser::Variable&>;

        /**
         * @return All references to the same variable value in the document,
         *         *including* the first one.
         */
        auto findAllReferences(const parser::Variable& varName) const
            -> std::generator<parser::Location>;

        /**
         * @brief Set the value of a variable
         */
        void set(const parser::Variable& var, const std::string& value);

        /**
         * @brief Compile variable settings into a document.
         *
         * @return A text document.
         */
        auto compile() const -> std::string;

        auto getLines() const -> const std::vector<std::string>&;

    private:
        /** Stores the entire document and all variables discovered during parsing. */
        parser::Result parseData;

        /** Stores substitution text for all locations that require substitution. */
        std::unordered_map<parser::Location, std::string> variableValues;
    };
} // namespace cloth
