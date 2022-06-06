#pragma once

#include <variant>

#include "TypeDefinition.h"
#include "ObjectDeclaration.h"

struct ImportStmt
{
    ImportStmt(Token _token)
        :
        token(std::move(_token)),
        importString(std::get<Token::StringValue>(token.value))
    {}

    Token token;
    std::string importString;
};

using Stmt = std::variant<
    ImportStmt,
    TypeDef,
    FieldDefinition
>;
