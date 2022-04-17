#pragma once

#include <variant>

#include "TypeDefinition.h"
#include "ObjectDeclaration.h"

using Stmt = std::variant<
    TypeDef,
    FieldDefinition
>;
