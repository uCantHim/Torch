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
    struct Object;
    struct Variated;
    using Value = std::variant<Literal, Object, Variated>;

    struct SingleValue
    {
        std::shared_ptr<Value> value;
    };

    struct MapValue
    {
        std::unordered_map<std::string, std::shared_ptr<Value>> values;
    };

    using FieldValueType = std::variant<SingleValue, MapValue>;

    struct Literal
    {
        std::string value;
    };

    struct Object
    {
        std::unordered_map<std::string, FieldValueType> fields;
    };

    struct Variated
    {
        struct Variant
        {
            std::vector<VariantFlag> setFlags;
            std::shared_ptr<Value> value;
        };

        std::vector<Variant> variants;
    };

    class ObjectConverter
    {
    public:
        explicit ObjectConverter(std::vector<Stmt> statements);

        auto convert() -> Object;

        auto getFlagTable() -> FlagTable&;

        void operator()(const TypeDef& def);
        void operator()(const EnumTypeDef& def);

        void operator()(const FieldDefinition&);

        auto operator()(const LiteralValue& val) -> std::shared_ptr<Value>;
        auto operator()(const Identifier& id) -> std::shared_ptr<Value>;
        auto operator()(const ObjectDeclaration& obj) -> std::shared_ptr<Value>;
        auto operator()(const MatchExpression& expr) -> std::shared_ptr<Value>;

    private:
        void error(std::string message);

        void setValue(const TypelessFieldName& fieldName, std::shared_ptr<Value> value);
        void setValue(const TypedFieldName& name, std::shared_ptr<Value> value);

        std::vector<Stmt> statements;
        Object globalObject;

        FlagTable flagTable;
        IdentifierTable identifierTable;
        VariantResolver resolver;

        Object* current{ &globalObject };
    };
} // namespace compiler
