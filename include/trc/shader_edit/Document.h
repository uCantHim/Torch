#pragma once

#include "Parser.h"
#include "VariableValue.h"

namespace shader_edit
{
    class Document
    {
    public:
        Document() = default;

        explicit Document(std::istream& is);
        explicit Document(std::vector<std::string> lines);
        explicit Document(ParseResult parseResult);

        /**
         * @brief Set the value of a variable
         */
        void set(const std::string& name, VariableValue value);

        /**
         * @brief Permutate on a variable
         */
        void permutate(const std::string& name, std::vector<VariableValue> values);

        /**
         * @brief Permutate on a variable
         */
        template<Renderable T>
        inline void permutate(const std::string& name, std::vector<T> values);

        /**
         * @brief Compile variable settings into one or more documents
         *
         * @return std::vector<std::string> Array of documents as strings
         */
        auto compile() const -> std::vector<std::string>;

    private:
        ParseResult parseData;

        std::unordered_map<std::string, VariableValue> singleValues;
        std::unordered_map<std::string, std::vector<VariableValue>> multiValues;
    };



    template<Renderable T>
    inline void Document::permutate(const std::string& name, std::vector<T> values)
    {
        std::vector<VariableValue> result;
        for (T& value : values) {
            result.emplace_back(std::move(value));
        }

        permutate(name, std::move(result));
    }
} // namespace shader_edit
