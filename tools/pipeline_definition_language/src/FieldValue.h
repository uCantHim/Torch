#pragma once

#include <memory>
#include <string>
#include <vector>
#include <optional>

#include "Terminals.h"
#include "MatchExpression.h"
#include "ObjectDeclaration.h"

class LiteralFieldValue;
class IdentifierFieldValue;
class ObjectDeclarationFieldValue;
class MatchExpressionFieldValue;

class FieldValueVisitor
{
public:
    virtual void visit(LiteralFieldValue& obj) = 0;
    virtual void visit(IdentifierFieldValue& obj) = 0;
    virtual void visit(ObjectDeclarationFieldValue& obj) = 0;
    virtual void visit(MatchExpressionFieldValue& obj) = 0;
};

/**
 * Base class for the 'field value' syntax-subtree
 */
class FieldValue
{
public:
    virtual void accept(FieldValueVisitor& vis) = 0;
};

class LiteralFieldValue : public FieldValue
{
public:
    LiteralFieldValue(LiteralValue val)
        : value(std::move(val))
    {}

    void accept(FieldValueVisitor& vis) override {
        vis.visit(*this);
    }

    LiteralValue value;
};

class IdentifierFieldValue : public FieldValue
{
public:
    IdentifierFieldValue(Identifier id)
        : identifier(std::move(id))
    {}

    void accept(FieldValueVisitor& vis) override {
        vis.visit(*this);
    }

    Identifier identifier;
};

class ObjectDeclarationFieldValue : public FieldValue
{
public:
    ObjectDeclarationFieldValue(ObjectDeclaration obj)
        : objectDeclaration(std::move(obj))
    {}

    void accept(FieldValueVisitor& vis) override {
        vis.visit(*this);
    }

    ObjectDeclaration objectDeclaration;
};

class MatchExpressionFieldValue : public FieldValue
{
public:
    MatchExpressionFieldValue(MatchExpression expr)
        : expr(std::move(expr))
    {}

    void accept(FieldValueVisitor& vis) override {
        vis.visit(*this);
    }

    MatchExpression expr;
};
