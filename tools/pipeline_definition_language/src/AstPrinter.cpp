#include "AstPrinter.h"

#include <iostream>



AstPrinter::AstPrinter(FieldValue& root)
    :
    root(&root)
{
}

void AstPrinter::print()
{
    root->accept(*this);
}

void AstPrinter::visit(LiteralFieldValue& val)
{
    std::cout << "literal";
}

void AstPrinter::visit(IdentifierFieldValue& val)
{
    std::cout << "identifier";
}

void AstPrinter::visit(ObjectDeclarationFieldValue& val)
{
    std::cout << "object:\n";
    ++indent;
    for (auto& field : val.objectDeclaration.fields)
    {
        printIndent();
        std::cout << "- ";
        std::visit([](auto& name) { std::cout << name.name.name; }, field.name);
        std::cout << ": ";
        field.value->accept(*this);
        std::cout << "\n";
    }
    --indent;
}

void AstPrinter::visit(MatchExpressionFieldValue& val)
{
    std::cout << "match expression";
}

void AstPrinter::printIndent()
{
    for (size_t i = 0; i < indent; i++) {
        std::cout << "    ";
    }
}
