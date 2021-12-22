#include "ShaderDocument.h"

#include <sstream>
#include <iomanip>



namespace shader_edit
{

ShaderDocument::ShaderDocument(std::istream& is)
    : ShaderDocument(parseShader(is))
{
}

ShaderDocument::ShaderDocument(std::vector<std::string> lines)
    : ShaderDocument(parseShader(std::move(lines)))
{
}

ShaderDocument::ShaderDocument(ParseResult parseResult)
    :
    parseData(std::move(parseResult))
{
}

void ShaderDocument::set(const std::string& name, VariableValue value)
{
    variableValues[name] = std::move(value);
}

auto ShaderDocument::permutate(const std::string& name, std::vector<VariableValue> values) const
    -> std::vector<ShaderDocument>
{
    std::vector<ShaderDocument> result;
    for (auto& value : values)
    {
        auto& copy = result.emplace_back(*this);
        copy.set(name, std::move(value));
    }

    return result;
}

auto ShaderDocument::compile() const -> std::string
{
    auto resultLines = parseData.lines;
    auto remainingVars = parseData.variablesByName;
    auto getVariableLine = [&remainingVars](const std::string& name) -> uint
    {
        auto var = remainingVars.extract(name);
        if (var.empty())
        {
            throw CompileError(
                "[In Document::compile]: Variable \"" + name + "\" does not exist "
                "in the document"
            );
        }

        return var.mapped();
    };

    // Replace variables in the document
    for (auto& [name, value] : variableValues)
    {
        resultLines.at(getVariableLine(name)) = value.toString();
    }

    // Ensure that all variables have been set
    if (!remainingVars.empty())
    {
        std::stringstream ss;
        ss << "[In Document::compile]: Unable to compile document - not all variables have"
           << " been set! Unset variables: ";
        for (const auto& [name, _] : remainingVars) ss << std::quoted(name) << "  ";

        throw CompileError(ss.str());
    }

    // Create resulting document string
    std::string result;
    for (auto& line : resultLines)
    {
        result += line;
        result += '\n';
    }

    return result;
}



auto permutate(const ShaderDocument& doc,
               const std::string& name,
               std::vector<VariableValue> values)
    -> std::vector<ShaderDocument>
{
    return doc.permutate(name, std::move(values));
}

auto permutate(const std::vector<ShaderDocument>& docs,
               const std::string& name,
               std::vector<VariableValue> values)
    -> std::vector<ShaderDocument>
{
    std::vector<ShaderDocument> result;
    for (const auto& doc : docs)
    {
        auto newDocs = permutate(doc, name, values);
        // Insert at end with move constructor
        for (auto& newDoc : newDocs) {
            result.emplace_back(std::move(newDoc));
        }
    }

    return result;
}

auto compile(const std::vector<ShaderDocument>& docs) -> std::vector<std::string>
{
    std::vector<std::string> result;
    for (const auto& doc : docs) {
        result.emplace_back(doc.compile());
    }
    return result;
}

} // namespace shader_edit
