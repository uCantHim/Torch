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


    /**
     * @brief Converts AST to semantically significant object representation
     */
    class ObjectConverter
    {
    public:
        explicit ObjectConverter(std::vector<Stmt> statements);

        auto convert() -> Object;

        auto getFlagTable() const -> const FlagTable&;
        auto getIdentifierTable() const -> const IdentifierTable&;

        void operator()(const TypeDef& def);
        void operator()(const EnumTypeDef& def);

        void operator()(const FieldDefinition&);

        auto operator()(const LiteralValue& val) -> std::shared_ptr<Value>;
        auto operator()(const Identifier& id) -> std::shared_ptr<Value>;
        auto operator()(const ListDeclaration& list) -> std::shared_ptr<Value>;
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
