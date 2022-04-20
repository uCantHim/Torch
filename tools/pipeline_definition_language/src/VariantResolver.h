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
    std::vector<FlagTable::FlagBitReference> setFlags;
    FieldValue value;  // Is guaranteed to be not a match expression
};

class VariantResolver
{
public:
    explicit VariantResolver(const FlagTable& flags);

    auto resolve(FieldValue& value) -> std::vector<FieldValueVariant>;

    auto operator()(const LiteralValue& val) const -> std::vector<FieldValueVariant>;
    auto operator()(const Identifier& id) const -> std::vector<FieldValueVariant>;
    auto operator()(const ObjectDeclaration& obj) const -> std::vector<FieldValueVariant>;
    auto operator()(const MatchExpression& expr) const -> std::vector<FieldValueVariant>;

private:
    static bool isVariantOfSameFlag(const FieldValueVariant& a,
                                    const FieldValueVariant& b);
    static void mergeFlags(std::vector<FlagTable::FlagBitReference>& dst,
                           const std::vector<FlagTable::FlagBitReference>& src);

    const FlagTable& flagTable;
};
