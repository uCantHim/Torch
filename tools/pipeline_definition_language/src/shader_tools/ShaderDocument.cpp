#include "shader_tools/ShaderDocument.h"

#include <format>
#include <list>
#include <ranges>
#include <sstream>

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

auto ShaderDocument::allVariables() -> std::generator<const std::string&>
{
    for (auto& [name, _] : parseData.variablesByName) {
        co_yield name;
    }
}

auto ShaderDocument::unsetVariables() -> std::generator<const std::string&>
{
    for (auto& [name, _] : parseData.variablesByName)
    {
        if (!variableValues.contains(name)) {
            co_yield name;
        }
    }
}

auto ShaderDocument::findOccurrences(const std::string& varName)
    -> std::generator<Location>
{
    auto it = parseData.variablesByName.find(varName);
    if (it != parseData.variablesByName.end())
    {
        auto locs = it->second | std::views::transform([](auto&& var){ return var.location; });
        co_yield std::ranges::elements_of(locs);
    }
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
    auto resultLines = parseData.lines;
    auto remainingVars = parseData.variablesInOrderOfOccurrence
                       | std::views::transform([](auto& v){ return v.name; })
                       | std::ranges::to<std::list>();

    // Replace variables in the document.
    // Process in reverse order to avoid indexing problems with multiple
    // variables on the same line, where character ranges of variables at the
    // end of the line would be invalidated when replacing leading variables.
    //
    // Example:
    //
    //     $a, $b       { a = "hello", b = "world" }
    //         v
    //     hellworld    { expected: "hello, world" }
    for (auto remIt = remainingVars.end();
         const auto& var : std::views::reverse(parseData.variablesInOrderOfOccurrence))
    {
        if (remIt != remainingVars.begin()) {
            --remIt;
        }
        if (variableValues.contains(var.name))
        {
            const auto& value = variableValues.at(var.name);
            const auto& [line, begin, end] = var.location;
            resultLines.at(line).replace(begin, end - begin, value.toString());

            remIt = remainingVars.erase(remIt);
        }
    }

    // Ensure that all variables have been set
    if (!allowUnsetVariables && !remainingVars.empty())
    {
        std::stringstream ss;
        ss << "[In Document::compile]: Unable to compile document - not all variables have"
           << " been set! Unset variables: [";
        ss << (remainingVars
                | std::views::transform([](auto&& s){ return std::format("\"{}\"", s); })
                | std::views::join_with(std::string{", "})
                | std::ranges::to<std::string>());
        ss << "]";

        throw CompileError(ss.str());
    }

    // Create resulting document string
    resultLines.emplace_back();  // The tests expect a newline at the end and I'm lazy
    return resultLines | std::views::join_with('\n') | std::ranges::to<std::string>();
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
