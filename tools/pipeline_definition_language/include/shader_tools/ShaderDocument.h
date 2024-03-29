#pragma once

#include <trc_util/Exception.h>

#include "ShaderDocumentParser.h"
#include "VariableValue.h"

namespace shader_edit
{
    class CompileError : public trc::Exception
    {
    public:
        CompileError(std::string message) : trc::Exception(std::move(message)) {}
    };

    /**
     * @brief A parsed document that contains variables
     */
    class ShaderDocument
    {
    public:
        ShaderDocument() = default;

        explicit ShaderDocument(std::istream& is);
        explicit ShaderDocument(const std::string& str);
        explicit ShaderDocument(ParseResult parseResult);

        /**
         * @brief Set the value of a variable
         */
        void set(const std::string& name, VariableValue value);

        /**
         * @brief Permutate on a variable
         */
        auto permutate(const std::string& name, std::vector<VariableValue> values) const
            -> std::vector<ShaderDocument>;

        /**
         * @brief Permutate on a variable
         */
        template<Renderable T>
        inline auto permutate(const std::string& name, std::vector<T> values) const
            -> std::vector<ShaderDocument>;

        /**
         * @brief Compile variable settings into one or more documents
         *
         * @param bool allowUnsetVariables If false, throw an exception if
         *        the document has one or more variables for which no value
         *        has been set via `set` or `permutate`.
         *
         * @return std::vector<std::string> Array of documents as strings
         * @throw CompileError if one or more variables in the shader file
         *                     have not been set via either `set` or
         *                     `permutate`.
         */
        auto compile(bool allowUnsetVariables = false) const -> std::string;

    private:
        /** Stores the entire document and all variables discovered during parsing. */
        ParseResult parseData;

        /**
         * Stores variable values that have been set with the `set` method.
         *
         * These will be read and used to replace the appropriate location
         * in the parsed document.
         */
        std::unordered_map<std::string, VariableValue> variableValues;
    };



    auto permutate(const ShaderDocument& doc,
                   const std::string& name,
                   const std::vector<VariableValue>& values)
        -> std::vector<ShaderDocument>;

    template<Renderable T>
    auto permutate(const ShaderDocument& doc,
                   const std::string& name,
                   std::vector<T> values)
        -> std::vector<ShaderDocument>;

    auto permutate(const std::vector<ShaderDocument>& docs,
                   const std::string& name,
                   const std::vector<VariableValue>& values)
        -> std::vector<ShaderDocument>;

    template<Renderable T>
    auto permutate(const std::vector<ShaderDocument>& docs,
                   const std::string& name,
                   std::vector<T> values)
        -> std::vector<ShaderDocument>;

    auto compile(const std::vector<ShaderDocument>& docs) -> std::vector<std::string>;



    ///////////////////////
    //  Implementations  //
    ///////////////////////

    template<Renderable T>
    inline auto permutate(const ShaderDocument& doc,
                          const std::string& name,
                          std::vector<T> values)
        -> std::vector<ShaderDocument>
    {
        std::vector<VariableValue> result;
        for (T& value : values) {
            result.emplace_back(std::move(value));
        }

        return permutate(doc, name, result);
    }

    template<Renderable T>
    inline auto permutate(const std::vector<ShaderDocument>& docs,
                          const std::string& name,
                          std::vector<T> values)
        -> std::vector<ShaderDocument>
    {
        std::vector<VariableValue> result;
        for (T& value : values) {
            result.emplace_back(std::move(value));
        }

        return permutate(docs, name, result);
    }

    template<Renderable T>
    inline auto ShaderDocument::permutate(const std::string& name, std::vector<T> values) const
        -> std::vector<ShaderDocument>
    {
        return permutate(*this, name, std::move(values));
    }
} // namespace shader_edit
