#include "shader_edit/Document.h"


namespace shader_edit
{

Document::Document(std::istream& is)
    : Document(parse(is))
{
}

Document::Document(std::vector<std::string> lines)
    : Document(parse(std::move(lines)))
{
}

Document::Document(ParseResult parseResult)
    :
    parseData(std::move(parseResult))
{
}

void Document::set(const std::string& name, VariableValue value)
{
    singleValues.try_emplace(name, std::move(value));
}

void Document::permutate(const std::string& name, std::vector<VariableValue> values)
{
    auto [it, success] = multiValues.try_emplace(name, std::move(values));
    if (!success)
    {
        throw std::runtime_error(
            "[In Document::permutate]: Variable " + name + " has already been mutated."
            " It may not be mutated again."
        );
    }
}

auto Document::compile() const -> std::vector<std::string>
{
    auto resultLines = parseData.lines;

    // Insert definite values
    for (auto& [name, value] : singleValues)
    {
        auto it = parseData.variablesByName.find(name);
        if (it == parseData.variablesByName.end())
        {
            throw std::runtime_error(
                "[In Document::compile]: Variable \"" + name + "\" does not exist in the document"
            );
        }

        resultLines.at(it->second) = value.toString() + "    // " + name;
    }

    std::string result;
    result.reserve(resultLines.size() * 100);
    for (auto& line : resultLines)
    {
        result += line;
        result += '\n';
    }

    return { std::move(result) };
}

} // namespace shader_edit
