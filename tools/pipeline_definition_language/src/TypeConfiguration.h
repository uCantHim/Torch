#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <variant>

using TypeName = std::string;

static const TypeName stringTypeName{ "String" };
static const TypeName globalObjectTypeName{ "__global" };
static const TypeName undefinedObjectType{ "<undefined object>" };

struct StringType
{
    static inline TypeName typeName{ stringTypeName };
};

enum class FieldType
{
    eSingleValue,
    eList,
    eMap
};

struct ObjectType
{
    struct Field
    {
        TypeName storedType;
        FieldType fieldType{ FieldType::eSingleValue };
    };

    TypeName typeName;
    std::unordered_map<std::string, Field> fields;
};

struct EnumType
{
    TypeName typeName;
    std::unordered_set<std::string> options;
};

using TypeType = std::variant<
    StringType,
    ObjectType,
    EnumType
>;

struct TypeConfiguration
{
    TypeConfiguration(std::unordered_map<TypeName, TypeType> types)
        : types(std::move(types))
    {}

    std::unordered_map<TypeName, TypeType> types;
};
