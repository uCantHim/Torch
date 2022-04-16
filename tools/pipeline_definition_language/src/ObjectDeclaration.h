#pragma once

#include <memory>
#include <vector>

#include "Terminals.h"

class FieldValue;

struct TypedFieldName
{
    Identifier type;
    Identifier name;
};

struct TypelessFieldName
{
    Identifier name;
};

struct FieldDefinition
{
    std::variant<TypelessFieldName, TypedFieldName> name;
    std::unique_ptr<FieldValue> value;
};

struct ObjectDeclaration
{
    std::vector<FieldDefinition> fields;
};
