#include "IdentifierTable.h"

#include "Util.h"



void IdentifierTable::registerIdentifier(const Identifier& id, FieldValue& val)
{
    variables.try_emplace(id, &val);
}

auto IdentifierTable::get(const Identifier& id) const -> const FieldValue*
{
    auto it = variables.find(id);
    if (it == variables.end()) {
        return nullptr;
    }
    return it->second;
}



auto IdentifierCollector::collect(const std::vector<Stmt>& statements) -> IdentifierTable
{
    for (auto& stmt : statements)
    {
        std::visit(*this, stmt);
    }

    return std::move(table);
}

void IdentifierCollector::operator()(const TypeDef& def)
{
    // Nothing
}

void IdentifierCollector::operator()(const FieldDefinition& def)
{
    if (std::holds_alternative<TypedFieldName>(def.name))
    {
        const auto& [type, name] = std::get<TypedFieldName>(def.name);
        table.registerIdentifier(name, *def.value);
    }
}
