#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <variant>

#include "SyntaxElements.h"
#include "FlagTable.h"
#include "IdentifierTable.h"
#include "VariantResolver.h"

namespace compiler
{
    struct Literal;
    struct List;
    struct Object;
    struct Variated;
    struct Reference;
    using Value = std::variant<Literal, List, Object, Variated, Reference>;


    //////////////////////////////
    //  Field type definitions  //
    //////////////////////////////

    struct SingleValue
    {
        std::shared_ptr<Value> value;
    };

    struct MapValue
    {
        std::unordered_map<std::string, std::shared_ptr<Value>> values;
    };

    using FieldValueType = std::variant<SingleValue, MapValue>;


    //////////////////////////////
    //  Value type definitions  //
    //////////////////////////////

    struct Literal
    {
        std::string value;
    };

    struct List
    {
        std::vector<std::shared_ptr<Value>> values;
    };

    struct Object
    {
        std::unordered_map<std::string, FieldValueType> fields;
    };

    struct Variated
    {
        struct Variant
        {
            VariantFlagSet setFlags;
            std::shared_ptr<Value> value;
        };

        std::vector<Variant> variants;
    };

    struct Reference
    {
        std::string name;
    };
} // namespace compiler
