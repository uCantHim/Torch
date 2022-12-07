#pragma once

#include "SemanticObject.h"

class ErrorReporter;

namespace compiler
{
    /**
     * @brief Converts AST to semantically significant object representation
     */
    class ObjectConverter
    {
    public:
        ObjectConverter(std::vector<Stmt> statements, ErrorReporter& errorReporter);

        auto convert() -> Object;

        auto getFlagTable() const -> const FlagTable&;
        auto getIdentifierTable() const -> const IdentifierTable&;

        void operator()(const ImportStmt&) {}
        void operator()(const TypeDef& def);
        void operator()(const EnumTypeDef& def);

        void operator()(const FieldDefinition&);

        auto operator()(const LiteralValue& val) -> std::shared_ptr<Value>;
        auto operator()(const Identifier& id) -> std::shared_ptr<Value>;
        auto operator()(const ListDeclaration& list) -> std::shared_ptr<Value>;
        auto operator()(const ObjectDeclaration& obj) -> std::shared_ptr<Value>;
        auto operator()(const MatchExpression& expr) -> std::shared_ptr<Value>;

    private:
        void error(const Token& token, std::string message);

        void setValue(const TypelessFieldName& fieldName, std::shared_ptr<Value> value);
        void setValue(const TypedFieldName& name, std::shared_ptr<Value> value);

        ErrorReporter* errorReporter;
        std::vector<Stmt> statements;
        Object globalObject;

        FlagTable flagTable;
        IdentifierTable identifierTable;
        VariantResolver resolver;

        Object* current{ &globalObject };
    };
} // namespace compiler
