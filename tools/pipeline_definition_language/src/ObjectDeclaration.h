#pragma once

#include <memory>
#include <vector>

#include "Terminals.h"
#include "FieldValue.h"

struct TypedFieldName
{
    Identifier type;
    Identifier name;
};

struct TypelessFieldName
{
    Identifier name;
};

using FieldName = std::variant<TypelessFieldName, TypedFieldName>;

struct FieldDefinition
{
    FieldName name;
    std::unique_ptr<FieldValue> value;
};

struct ObjectDeclaration
{
    ObjectDeclaration(Token token) : token(std::move(token)) {}

    Token token;
    std::vector<FieldDefinition> fields;
};
