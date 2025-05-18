#include "document.h"

#include <algorithm>
#include <ranges>



namespace cloth
{

Document::Document(parser::Result _parseResult)
    :
    parseData(std::move(_parseResult))
{
}

auto Document::allVariables() const -> std::generator<const parser::Variable&>
{
    co_yield std::ranges::elements_of(parseData.variablesInOrderOfOccurrence);
}

auto Document::findAllReferences(const parser::Variable& var) const
    -> std::generator<parser::Location>
{
    co_yield var.location;
    for (const auto& loc : parseData.allReferences.at(var.fullDeclText)) {
        co_yield loc;
    }
}

void Document::set(const parser::Variable& var, const std::string& value)
{
    for (const auto& ref : parseData.allReferences.at(var.fullDeclText)) {
        variableValues[ref] = value;
    }
}

auto Document::compile() const -> std::string
{
    auto resultLines = parseData.lines;
    auto values = variableValues
        | std::ranges::to<std::vector<std::pair<parser::Location, std::string>>>();
    std::ranges::sort(values, [](auto& a, auto& b){ return a.first < b.first; });

    // Replace variables in the document.
    // All of these fancy-looking calculations are here to ensure that inserted
    // text of different size than the text it replaces correctly changes the
    // boundaries of following substitutions in the same line.
    //
    // Initially, I simply processed the substitutions in reverse order of
    // occurrence, but that doesn't work anymore when we have to detect and skip
    // nested substitutions (arguments).
    //
    // Example (if this mechanism weren't in place):
    //
    //     $a, $b       { a = "hello", b = "world" }
    //         v
    //     hellworld    { expected: "hello, world" }
    std::optional<parser::Location> lastLoc;
    int lineSizeChange = 0;
    for (const auto& [loc, value] : values)
    {
        // Skip nested variables (arguments). They don't receive code
        // substitution because their values are included in the parent's value.
        if (lastLoc)
        {
            const auto [line, begin, end] = *lastLoc;
            if (loc.line == line && loc.firstChar >= begin && loc.endChar <= end) {
                continue;
            }
        }
        if (lastLoc && loc.line != lastLoc->line) {
            lineSizeChange = 0;
        }

        auto [line, begin, end] = loc;
        begin += lineSizeChange;
        end += lineSizeChange;
        assert(resultLines.at(loc.line).size() >= loc.endChar + lineSizeChange);
        resultLines.at(line).replace(begin, end - begin, value);

        lineSizeChange += int(value.size()) - int(end - begin);
        lastLoc = loc;
    }

    // Create resulting document string
    resultLines.emplace_back();  // The tests expect a newline at the end and I'm lazy
    return resultLines | std::views::join_with('\n') | std::ranges::to<std::string>();
}

auto Document::getLines() const -> const std::vector<std::string>&
{
    return parseData.lines;
}

} // namespace shader_edit
