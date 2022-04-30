#pragma once

#include <vector>

#include "FieldValue.h"

struct ListDeclaration
{
    ListDeclaration(Token token) : token(std::move(token)) {}

    Token token;
    std::vector<FieldValue> items;
};
