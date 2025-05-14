#include "document.h"

#include <ranges>



namespace cloth
{

Document::Document(parser::Result _parseResult)
    :
    parseData(std::move(_parseResult))
{
    doc = parseData.toDocument();
}

auto Document::allVariables() -> std::generator<const FullId&>
{
    for (auto& [name, _] : parseData.variablesByName) {
        co_yield name;
    }
}

auto Document::unsetVariables() -> std::generator<const FullId&>
{
    for (const auto& name : doc.unsetVariables()) {
        co_yield FullId{ name };
    }
}

auto Document::findOccurrences(const FullId& varName)
    -> std::generator<parser::Location>
{
    auto it = parseData.variablesByName.find(varName);
    if (it != parseData.variablesByName.end())
    {
        auto locs = it->second | std::views::transform([](auto&& var){ return var.location; });
        co_yield std::ranges::elements_of(locs);
    }
}

void Document::set(const FullId& name, std::string value)
{
    doc.set(name.id, std::move(value));
}

auto Document::compile(bool allowUnsetVariables) const
    -> std::expected<std::string, DocumentError>
{
    try {
        return doc.compile(allowUnsetVariables);
    }
    catch (shader_edit::CompileError& err) {
        return std::unexpected(DocumentError{ err.what() });
    }
}

} // namespace shader_edit
