#pragma once

#include <vector>
#include <unordered_map>
#include <functional>

#include "SyntaxElements.h"
#include "TypeConfiguration.h"
#include "CompileResult.h"
#include "FlagTable.h"
#include "IdentifierTable.h"
#include "VariantResolver.h"

class ErrorReporter;

class Compiler
{
public:
    Compiler(std::vector<Stmt> statements, ErrorReporter& errorReporter);

    auto compile() -> CompileResult;

    void operator()(const TypeDef& def);
    void operator()(const EnumTypeDef& def);

    void operator()(const FieldDefinition&);
    void operator()(const LiteralValue& val);
    void operator()(const Identifier& id);
    void operator()(const ObjectDeclaration& obj);
    void operator()(const MatchExpression& expr);

private:
    auto getIdentifierID(const Identifier& id) -> uint;

    const std::vector<Stmt> statements;

    IdentifierTable identifierTable;
    FlagTable flagTable;
    VariantResolver resolver;
    std::unordered_map<TypeName, std::function<void(FieldValue&)>> typeCompileFuncs;

    std::unordered_map<Identifier, FieldValue*> variables;

    ErrorReporter* errorReporter;
};
