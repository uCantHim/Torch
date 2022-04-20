#pragma once

#include <vector>
#include <unordered_map>
#include <variant>

#include "SyntaxElements.h"
#include "FlagTable.h"

/**
 * @brief A FieldValue specialized for a specific combination of flag bits
 */
struct FieldValueVariant
{
    using Value = std::variant<
        LiteralValue,
        Identifier,
        ObjectDeclaration
    >;

    std::vector<FlagTable::FlagBitReference> setFlags;
    Value value;
};

class VariantResolver
{
public:
    explicit VariantResolver(const FlagTable& flags);

    auto resolve(FieldValue& value) -> std::vector<FieldValueVariant>;

private:
    const FlagTable& flagTable;
};
