#include "CompilerObjectRepresentation.h"

#include "Exceptions.h"



namespace compiler
{

ObjectConverter::ObjectConverter(std::vector<Stmt> _statements)
    :
    statements(std::move(_statements)),
    flagTable(FlagTypeCollector{}.collect(statements)),
    identifierTable(IdentifierCollector{}.collect(statements)),
    resolver(flagTable, identifierTable)
{
}

auto ObjectConverter::convert() -> Object
{
    for (const auto& stmt : statements)
    {
        std::visit(*this, stmt);
    }

    return std::move(globalObject);
}

auto ObjectConverter::getFlagTable() const -> const FlagTable&
{
    return flagTable;
}

auto ObjectConverter::getIdentifierTable() const -> const IdentifierTable&
{
    return identifierTable;
}

void ObjectConverter::operator()(const TypeDef& def)
{
    std::visit(*this, def);
}

void ObjectConverter::operator()(const EnumTypeDef&)
{
    // Nothing - enum types are handled by FlagTypeCollector
}

void ObjectConverter::operator()(const FieldDefinition& def)
{
    const auto& [name, value] = def;
    auto variants = resolver.resolve(*value);
    if (variants.size() > 1)
    {
        Variated variated;
        for (auto& var : variants)
        {
            variated.variants.emplace_back(
                var.setFlags,
                std::visit(*this, var.value)
            );
        }

        auto val = std::make_shared<Value>(variated);
        std::visit([this, val](auto& name){ setValue(name, val); }, name);
    }
    else {
        auto val = std::visit(*this, variants.front().value);
        std::visit([this, val](auto& name){ setValue(name, val); }, name);
    }
}

auto ObjectConverter::operator()(const LiteralValue& val) -> std::shared_ptr<Value>
{
    return std::make_shared<Value>(Literal{ .value=val.value });
}

auto ObjectConverter::operator()(const Identifier& id) -> std::shared_ptr<Value>
{
    return std::make_shared<Value>(Reference{ id.name });
}

auto ObjectConverter::operator()(const ObjectDeclaration& obj) -> std::shared_ptr<Value>
{
    Object object;
    Object* old = current;
    current = &object;

    for (const auto& field : obj.fields)
    {
        (*this)(field);
    }

    current = old;
    return std::make_shared<Value>(std::move(object));
}

auto ObjectConverter::operator()(const MatchExpression&) -> std::shared_ptr<Value>
{
    throw InternalLogicError("AST contains a match expression after VariantResolver has been"
                             " called on it. This should not be possible.");
}

void ObjectConverter::error(std::string message)
{
    throw InternalLogicError(std::move(message));
}

void ObjectConverter::setValue(
    const TypelessFieldName& fieldName,
    std::shared_ptr<Value> value)
{
    auto [_, success] = current->fields.try_emplace(
        fieldName.name.name,
        SingleValue{ std::move(value) }
    );

    if (!success) {
        error("Duplicate object property \"" + fieldName.name.name + "\".");
    }
}

void ObjectConverter::setValue(
    const TypedFieldName& name,
    std::shared_ptr<Value> value)
{
    const auto& [fieldName, mapName] = name;
    auto [it, _0] = current->fields.try_emplace(fieldName.name, MapValue{});
    auto& map = std::get<MapValue>(it->second);

    auto [_1, success] = map.values.try_emplace(mapName.name, value);
    if (!success) {
        error("Duplicate entry \"" + mapName.name + "\" in field \"" + fieldName.name + "\".");
    }
}

} // namespace compiler
