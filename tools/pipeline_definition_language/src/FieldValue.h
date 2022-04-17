#pragma once

#include <variant>

#include "Terminals.h"

class ObjectDeclaration;
class MatchExpression;

using FieldValue = std::variant<
    LiteralValue,
    Identifier,
    ObjectDeclaration,
    MatchExpression
>;
