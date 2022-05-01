#pragma once

#include "Util.h"
#include "TypeConfiguration.h"
#include "SyntaxElements.h"

class ErrorReporter;

class TypeParser
{
public:
    TypeParser(TypeConfiguration& out, ErrorReporter& errorReporter);

    void parse(const std::vector<Stmt>& statements);

    void operator()(const TypeDef& def);
    void operator()(const EnumTypeDef& def);
    void operator()(const FieldDefinition&);

private:
    TypeConfiguration* out;
    ErrorReporter* errorReporter;
};
