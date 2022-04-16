#pragma once

#include <string>

#include "Token.h"

/**
 * We only have string literal types
 */
struct LiteralValue
{
    std::string value;
};

/**
 * @brief An identifier is just an arbitrary string name
 */
struct Identifier
{
    std::string name;
};
