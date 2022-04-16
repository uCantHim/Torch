#pragma once

#include "SyntaxElements.h"

class AstPrinter : public FieldValueVisitor
{
public:
    explicit AstPrinter(FieldValue& root);

    void print();

    void visit(LiteralFieldValue& val) override;
    void visit(IdentifierFieldValue& val) override;
    void visit(ObjectDeclarationFieldValue& val) override;
    void visit(MatchExpressionFieldValue& val) override;

private:
    void printIndent();

    FieldValue* root;
    size_t indent{ 0 };
};
