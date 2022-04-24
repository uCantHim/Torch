#include "AstPrinter.h"

#include <iostream>



AstPrinter::AstPrinter(std::vector<Stmt> statements)
    :
    statements(std::move(statements))
{
}

void AstPrinter::print()
{
    for (auto& stmt : statements) {
        std::visit(*this, stmt);
    }
}

void AstPrinter::operator()(TypeDef& def)
{
    std::visit(*this, def);
}

void AstPrinter::operator()(EnumTypeDef& def)
{
    std::cout << "enum " << def.name << ": ";
    for (auto& opt : def.options) {
        std::cout << opt << " | ";
    }
    std::cout << "\b\b \n\n";
}

void AstPrinter::operator()(FieldDefinition& field)
{
    printField(field);
}

void AstPrinter::operator()(LiteralValue& val)
{
    std::cout << "literal [" << val.value << "]";
}

void AstPrinter::operator()(Identifier& id)
{
    std::cout << "identifier [" << id.name << "]";
}

void AstPrinter::operator()(ObjectDeclaration& val)
{
    std::cout << "object:\n";
    ++indent;
    for (auto& field : val.fields)
    {
        printField(field);
    }
    --indent;
}

void AstPrinter::operator()(MatchExpression& expr)
{
    std::cout << "match on " << expr.matchedType.name << "\n";
    ++indent;
    for (auto& opt : expr.cases)
    {
        printIndent();
        std::cout << opt.caseIdentifier.name << " -> ";
        std::visit(*this, *opt.value);
        std::cout << "\n";
    }
    --indent;
}

void AstPrinter::printField(FieldDefinition& field)
{
    printIndent();
    std::cout << "- ";
    std::visit([](auto& name) { std::cout << name.name.name; }, field.name);
    std::cout << ": ";
    std::visit(*this, *field.value);
    std::cout << "\n";
}

void AstPrinter::printIndent()
{
    for (size_t i = 0; i < indent; i++) {
        std::cout << "    ";
    }
}
