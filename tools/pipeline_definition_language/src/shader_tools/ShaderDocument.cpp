#include "shader_tools/ShaderDocument.h"

#include <sstream>
#include <iomanip>

#include <trc_util/StringManip.h>



namespace shader_edit
{

ShaderDocument::ShaderDocument(std::istream& is)
    : ShaderDocument(parseShader(is))
{
}

ShaderDocument::ShaderDocument(const std::string& str)
    : ShaderDocument(parseShader(trc::util::splitString(str, '\n')))
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

auto ShaderDocument::compile(bool allowUnsetVariables) const -> std::string
{
    /** A variable's location */
    struct Location
    {
        uint line;
        size_t begin;
        size_t end;
    };

    auto resultLines = parseData.lines;
    auto remainingVars = parseData.variablesByName;
    auto getVariableLine = [&remainingVars](const std::string& name) -> Location
    {
        auto node = remainingVars.extract(name);
        if (node.empty())
        {
            throw CompileError("[In Document::compile]: Variable \"" + name + "\" does not exist"
                               " in the document");
        }

        const auto& var = node.mapped();
        return { var.line, var.firstChar, var.lastChar };
    };

    // Replace variables in the document
    for (auto& [name, value] : variableValues)
    {
        auto [line, begin, end] = getVariableLine(name);
        resultLines.at(line).replace(begin, end - begin, value.toString());
    }

    // Ensure that all variables have been set
    if (!allowUnsetVariables && !remainingVars.empty())
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
               const std::vector<VariableValue>& values)
    -> std::vector<ShaderDocument>
{
    return doc.permutate(name, values);
}

auto permutate(const std::vector<ShaderDocument>& docs,
               const std::string& name,
               const std::vector<VariableValue>& values)
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
    result.reserve(docs.size());
    for (const auto& doc : docs) {
        result.emplace_back(doc.compile());
    }
    return result;
}

} // namespace shader_edit
