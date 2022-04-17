#pragma once

#include "SyntaxElements.h"

class AstPrinter
{
public:
    explicit AstPrinter(FieldValue& root);

    void print();

    void operator()(LiteralValue&);
    void operator()(Identifier&);
    void operator()(ObjectDeclaration&);
    void operator()(MatchExpression&);

private:
    void printIndent();

    FieldValue* root;
    size_t indent{ 0 };
};
