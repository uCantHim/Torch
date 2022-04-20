#pragma once

#include "SyntaxElements.h"
#include "TypeConfiguration.h"

class ErrorReporter;

struct FileContents
{
    std::unordered_map<std::string, FieldValue> identifiersToValues;
};

auto makeDefaultTypeConfig() -> TypeConfiguration;

class TypeChecker
{
public:
    TypeChecker(TypeConfiguration config, ErrorReporter& errorReporter);

    bool check(const std::vector<Stmt>& statements);

    void operator()(const TypeDef&);
    void operator()(const EnumTypeDef&);
    void operator()(const FieldDefinition&);

private:
    void checkFieldDefinition(const ObjectType& parent, const FieldDefinition& def);

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
        bool operator()(const ObjectDeclaration&) const;
        bool operator()(const MatchExpression&) const;

    private:
        void expectedTypeError(const Token& token, const TypeName& actualType) const;

        const TypeType* const expectedType;
        const TypeName expectedTypeName;
        TypeChecker* const self;

        const Token* currentToken;
    };

    static auto getToken(const FieldValue& val) -> const Token&;

    /** @brief Register an error at the error reporter */
    void error(const Token& token, std::string message);

    TypeConfiguration config;
    std::unordered_map<Identifier, FieldValue*> identifierValues;

    ErrorReporter* errorReporter;
};
