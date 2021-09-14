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

        void set(const std::string& name, VariableValue value);
        void permutate(const std::string& name, std::vector<VariableValue> values);

        auto compile() const -> std::vector<std::string>;

    private:
        ParseResult parseData;

        std::unordered_map<std::string, VariableValue> singleValues;
        std::unordered_map<std::string, std::vector<VariableValue>> multiValues;
    };
} // namespace shader_edit
