#include "TypeParser.h"

#include "ErrorReporter.h"



TypeParser::TypeParser(TypeConfiguration& out, ErrorReporter& errorReporter)
    :
    out(&out),
    errorReporter(&errorReporter)
{
}

void TypeParser::parse(const std::vector<Stmt>& statements)
{
    for (const auto& stmt : statements) {
        std::visit(*this, stmt);
    }
}

void TypeParser::operator()(const TypeDef& def)
{
    std::visit(*this, def);
}

void TypeParser::operator()(const EnumTypeDef& def)
{
    EnumType type{ .typeName=def.name, .options={} };
    for (const auto& opt : def.options) {
        type.options.emplace(opt.value);
    }

    auto [it, success] = out->types.try_emplace(def.name, std::move(type));
    if (!success) {
        errorReporter->error(Error{
            .location=def.token.location,
            .message="Duplicate definition of type \"" + def.name + "\"."
        });
    }
}

void TypeParser::operator()(const FieldDefinition&)
{
    // Nothing
}
