#pragma once

#include <memory>
#include <vector>

#include "Terminals.h"
#include "FieldValue.h"

struct TypedFieldName
{
    Identifier name;
    Identifier mappedName;
};

struct TypelessFieldName
{
    Identifier name;
};

using FieldName = std::variant<TypelessFieldName, TypedFieldName>;

inline auto getFieldName(const FieldName& name)
{
    return std::visit(
        [](const auto& v) -> const std::string& { return v.name.name; },
        name
    );
}

struct FieldDefinition
{
    FieldName name;
    std::shared_ptr<FieldValue> value;
};

struct ObjectDeclaration
{
    ObjectDeclaration(Token token) : token(std::move(token)) {}

    Token token;
    std::vector<FieldDefinition> fields;
};
