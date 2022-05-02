#include "IdentifierTable.h"

#include "ErrorReporter.h"
#include "Util.h"



void IdentifierTable::registerIdentifier(const Identifier& id, IdentifierValue val)
{
    auto [_, success] = table.try_emplace(id, val);
    if (!success) {
        throw DuplicateIdentifierError(id);
    }
}

bool IdentifierTable::has(const Identifier& id) const
{
    return table.contains(id);
}

auto IdentifierTable::get(const Identifier& id) const -> const IdentifierValue&
{
    auto it = table.find(id);
    if (it == table.end()) {
        throw std::out_of_range("Identifier \"" + id.name + "\" is not present in table.");
    }
    return it->second;
}

template<typename T>
auto IdentifierTable::get(const Identifier& id) const -> const T*
{
    const auto& type = get(id);
    try {
        return &std::get<T>(type);
    }
    catch (const std::bad_variant_access&) {
        return nullptr;
    }
}

auto IdentifierTable::getValueReference(const Identifier& id) const -> const ValueReference*
{
    return get<ValueReference>(id);
}

auto IdentifierTable::getTypeName(const Identifier& id) const -> const TypeName*
{
    return get<TypeName>(id);
}

auto IdentifierTable::getDataConstructor(const Identifier& id) const -> const DataConstructor*
{
    return get<DataConstructor>(id);
}



IdentifierCollector::IdentifierCollector(ErrorReporter& errorReporter)
    :
    errorReporter(&errorReporter)
{
}

auto IdentifierCollector::collect(const std::vector<Stmt>& statements) -> IdentifierTable
{
    for (auto& stmt : statements)
    {
        try {
            std::visit(*this, stmt);
        }
        catch (const DuplicateIdentifierError& err) {
            errorReporter->error(Error{
                .location=err.identifier.token.location,
                .message="Redefinition of \"" + err.identifier.name + "\""
            });
        }
    }

    return std::move(table);
}

void IdentifierCollector::operator()(const TypeDef& def)
{
    std::visit(*this, def);
}

void IdentifierCollector::operator()(const EnumTypeDef& def)
{
    table.registerIdentifier(Identifier{ def.token }, TypeName{ def.name });
    for (const auto& opt : def.options)
    {
        table.registerIdentifier(
            Identifier{ opt.token },
            DataConstructor{ .constructedType=def.name, .dataName=opt.value }
        );
    }
}

void IdentifierCollector::operator()(const FieldDefinition& def)
{
    if (std::holds_alternative<TypedFieldName>(def.name))
    {
        const auto& [type, name] = std::get<TypedFieldName>(def.name);
        table.registerIdentifier(name, ValueReference{ .referencedValue=def.value.get() });
    }
}
