#pragma once

#include <vector>

#include "Token.h"
#include "FieldValue.h"
#include "MatchExpression.h"
#include "ObjectDeclaration.h"

struct ListDeclaration
{
    ListDeclaration(Token token) : token(std::move(token)) {}

    Token token;
    std::vector<FieldValue> items;
};
