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
    auto remainingVars = parseData.variablesByName;
    auto getVariableLine = [&remainingVars](const std::string& name) -> uint
    {
        auto var = remainingVars.extract(name);
        if (var.empty())
        {
            throw CompileError(
                "[In Document::compile]: Variable \"" + name + "\" does not exist in the document"
            );
        }

        return var.mapped();
    };


    auto resultLines = parseData.lines;

    // Insert definite values
    for (auto& [name, value] : singleValues)
    {
        resultLines.at(getVariableLine(name)) = value.toString() + "    // " + name;
    }

    // Create permutations
    auto createPermutations = [&](const auto& documents, const auto& name, const auto& values)
    {
        const uint line = getVariableLine(name);
        std::vector<ParseResult> documentCopies;
        // Values first. This way, the first specified permutation remains
        // on the outer nesting level.
        for (const auto& value : values)
        {
            for (const auto& doc : documents)
            {
                auto& copy = documentCopies.emplace_back(doc);
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

    // Ensure that all variables have been set
    if (!remainingVars.empty())
    {
        std::stringstream ss;
        ss << "[In Document::compile]: Unable to compile document - not all variables have"
           << " been set! Unset variables: ";
        for (const auto& [name, _] : remainingVars) ss << std::quoted(name) << "  ";

        throw CompileError(ss.str());
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
