#include "shader_edit/Document.h"

#include <sstream>
#include <iomanip>



namespace shader_edit
{

Document::Document(std::istream& is)
    : Document(parseShader(is))
{
}

Document::Document(std::vector<std::string> lines)
    : Document(parseShader(std::move(lines)))
{
}

Document::Document(ParseResult parseResult)
    :
    parseData(std::move(parseResult))
{
}

void Document::set(const std::string& name, VariableValue value)
{
    variableValues[name] = std::move(value);
}

auto Document::permutate(const std::string& name, std::vector<VariableValue> values) const
    -> std::vector<Document>
{
    std::vector<Document> result;
    for (auto& value : values)
    {
        auto& copy = result.emplace_back(*this);
        copy.set(name, std::move(value));
    }

    return result;
}

auto Document::compile() const -> std::string
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



auto permutate(const Document& doc,
               const std::string& name,
               std::vector<VariableValue> values)
    -> std::vector<Document>
{
    return doc.permutate(name, std::move(values));
}

auto permutate(const std::vector<Document>& docs,
               const std::string& name,
               std::vector<VariableValue> values)
    -> std::vector<Document>
{
    std::vector<Document> result;
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

auto compile(const std::vector<Document>& docs) -> std::vector<std::string>
{
    std::vector<std::string> result;
    for (const auto& doc : docs) {
        result.emplace_back(doc.compile());
    }
    return result;
}

} // namespace shader_edit
