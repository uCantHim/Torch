#pragma once

#include <variant>

#include "Terminals.h"

struct ListDeclaration;
struct ObjectDeclaration;
struct MatchExpression;

using FieldValue = std::variant<
    LiteralValue,
    Identifier,
    ListDeclaration,
    ObjectDeclaration,
    MatchExpression
>;
