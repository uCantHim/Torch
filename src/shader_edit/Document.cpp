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
    singleValues[name] = std::move(value);
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

    // Create permutations
    auto createPermutations = [](const auto& documents, const auto& name, const auto& values) {
        std::vector<ParseResult> documentCopies;
        // Values first. This way, the first specified permutation remains
        // on the outer nesting level.
        for (const auto& value : values)
        {
            for (const auto& doc : documents)
            {
                auto& copy = documentCopies.emplace_back(doc);
                const uint line = copy.variablesByName.at(name);
                copy.lines.at(line) = value.toString();
            }
        }

        return documentCopies;
    };

    std::vector<ParseResult> permutations{ parseData };  // Create one initial copy
    permutations[0].lines = resultLines;
    for (auto& [name, values] : multiValues)
    {
        permutations = createPermutations(permutations, name, values);
    }

    // Create resulting documents
    std::vector<std::string> results;
    for (const auto& doc : permutations)
    {
        auto& result = results.emplace_back();
        result.reserve(doc.lines.size() * 100);
        for (auto& line : doc.lines)
        {
            result += line;
            result += '\n';
        }
    }

    return results;
}

} // namespace shader_edit
