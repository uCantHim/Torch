#pragma once

#include <cassert>
#include <memory>
#include <vector>

#include "Terminals.h"
#include "FieldValue.h"

struct MatchCase
{
    Identifier caseIdentifier;
    std::unique_ptr<FieldValue> value;
};

struct MatchExpression
{
    MatchExpression(Token _token)
        : token(std::move(_token)), matchedType(token)
    {
        assert(token.type == TokenType::eIdentifier && "Token must be the matched type");
    }

    Token token;
    Identifier matchedType;
    std::vector<MatchCase> cases;
};
