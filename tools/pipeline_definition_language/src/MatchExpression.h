#pragma once

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
    Identifier matchedType;
    std::vector<MatchCase> cases;
};
