#include "Compiler.h"

#include "Exceptions.h"



Compiler::Compiler(std::vector<Stmt> _statements, ErrorReporter& errorReporter)
    :
    statements(std::move(_statements)),
    flagTable(FlagTypeCollector{}.collect(statements)),
    identifierTable(IdentifierCollector{}.collect(statements)),
    resolver(flagTable, identifierTable),
    errorReporter(&errorReporter)
{
}

auto Compiler::compile() -> CompileResult
{
    CompileResult result;

    for (const auto& stmt : statements)
    {
        std::visit(*this, stmt);
    }

    result.flagTypes = flagTable.getAllFlags();

    return result;
}

void Compiler::operator()(const TypeDef& def)
{
    std::visit(*this, def);
}

void Compiler::operator()(const EnumTypeDef&)
{
    // Nothing - enum types are handled by FlagTypeCollector
}

void Compiler::operator()(const FieldDefinition& def)
{
    const auto& [name, value] = def;

    if (std::holds_alternative<TypelessFieldName>(name)) return;

    auto funcIt = typeCompileFuncs.find(std::get<TypedFieldName>(name).type.name);
    if (funcIt != typeCompileFuncs.end())
    {
        auto variants = resolver.resolve(*value);
        for (auto& var : variants)
        {
            funcIt->second(var.value);
            std::visit(*this, var.value);
        }
    }
    else {
    }
}

void Compiler::operator()(const LiteralValue& val)
{
}

void Compiler::operator()(const Identifier& id)
{
    throw InternalLogicError("AST contains an identifier after VariantResolver has been"
                             " called on it. This should not be possible.");
}

void Compiler::operator()(const ObjectDeclaration& obj)
{
}

void Compiler::operator()(const MatchExpression& expr)
{
    throw InternalLogicError("AST contains a match expression after VariantResolver has been"
                             " called on it. This should not be possible.");
}
