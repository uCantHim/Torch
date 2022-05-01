#pragma once

#include "SyntaxElements.h"
#include "TypeConfiguration.h"
#include "IdentifierTable.h"

class ErrorReporter;

struct FileContents
{
    std::unordered_map<std::string, FieldValue> identifiersToValues;
};

class TypeChecker
{
public:
    TypeChecker(TypeConfiguration config, ErrorReporter& errorReporter);

    bool check(const std::vector<Stmt>& statements);

    void operator()(const TypeDef&);
    void operator()(const FieldDefinition&);

private:
    static auto getToken(const FieldName& name) -> const Token&;
    static auto getToken(const FieldValue& val) -> const Token&;

    auto inferType(const ObjectType& parent, const std::string& field) -> const ObjectType::Field&;

    void checkFieldDefinition(const ObjectType& parent,
                              const FieldDefinition& def,
                              bool allowArbitraryFields = false);

    /**
     * Defines ad-hoc state on the stack to recursively check the types of
     * single field values.
     *
     * This avoids some mutable 'current*' state fields in TypeChecker.
     */
    struct CheckValueType
    {
        CheckValueType(TypeType& expected, TypeChecker& self);

        bool operator()(const LiteralValue&) const;
        bool operator()(const Identifier&) const;
        bool operator()(const ListDeclaration&) const;
        bool operator()(const ObjectDeclaration&) const;
        bool operator()(const MatchExpression&) const;

    private:
        void expectedTypeError(const Token& token, const TypeName& actualType) const;

        const TypeType* const expectedType;
        const TypeName expectedTypeName;
        TypeChecker* const self;
    };

    /** @brief Register an error at the error reporter */
    void error(const Token& token, std::string message);

    TypeConfiguration config;
    IdentifierTable identifierTable;

    ErrorReporter* errorReporter;
};
