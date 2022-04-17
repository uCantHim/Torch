#pragma once

#include "SyntaxElements.h"

class AstPrinter
{
public:
    explicit AstPrinter(std::vector<Stmt> statements);

    void print();

    void operator()(TypeDef&);
    void operator()(FieldDefinition&);

    void operator()(LiteralValue&);
    void operator()(Identifier&);
    void operator()(ObjectDeclaration&);
    void operator()(MatchExpression&);

private:
    void printField(FieldDefinition& field);
    void printIndent();

    std::vector<Stmt> statements;
    size_t indent{ 0 };
};
