#pragma once

#include <vector>
#include <unordered_map>

#include "SyntaxElements.h"

/**
 * @brief Saves top-level variable declarations with their values
 */
class IdentifierTable
{
public:
    void registerIdentifier(const Identifier& id, FieldValue& val);

    auto get(const Identifier& id) const -> const FieldValue*;

private:
    std::unordered_map<Identifier, FieldValue*> variables;
};

/**
 * @brief Collects all top-level values qualified with a typed name
 * ('global variables') and saves them in a lookup table.
 */
class IdentifierCollector
{
public:
    auto collect(const std::vector<Stmt>& statements) -> IdentifierTable;

    void operator()(const TypeDef& def);
    void operator()(const FieldDefinition&);

private:
    IdentifierTable table;
};
