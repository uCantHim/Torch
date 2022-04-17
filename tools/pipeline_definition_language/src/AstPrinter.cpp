#include "AstPrinter.h"

#include <iostream>



AstPrinter::AstPrinter(FieldValue& root)
    :
    root(&root)
{
}

void AstPrinter::print()
{
    std::visit(*this, *root);
}

void AstPrinter::operator()(LiteralValue&)
{
    std::cout << "literal";
}

void AstPrinter::operator()(Identifier&)
{
    std::cout << "identifier";
}

void AstPrinter::operator()(ObjectDeclaration& val)
{
    std::cout << "object:\n";
    ++indent;
    for (auto& field : val.fields)
    {
        printIndent();
        std::cout << "- ";
        std::visit([](auto& name) { std::cout << name.name.name; }, field.name);
        std::cout << ": ";
        std::visit(*this, *field.value);
        std::cout << "\n";
    }
    --indent;
}

void AstPrinter::operator()(MatchExpression&)
{
    std::cout << "match expression";
}

void AstPrinter::printIndent()
{
    for (size_t i = 0; i < indent; i++) {
        std::cout << "    ";
    }
}
