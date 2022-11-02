#pragma once

#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <vector>

#include "SyntaxElements.h"
#include "FlagTable.h"

class IdentifierTable;

/**
 * @brief A FieldValue specialized for a specific combination of flag bits
 */
struct FieldValueVariant
{
    VariantFlagSet setFlags;
    FieldValue value;  // Is guaranteed to be not a match expression
};

/**
 * @brief Resolves variated field values
 *
 * Creates complete value declarations by resolving variations and
 * following references.
 */
class VariantResolver
{
public:
    class ValueVariantSet
    {
    public:
        ValueVariantSet(std::vector<FieldValueVariant> vars);

        auto begin() { return variants.begin(); }
        auto end() { return variants.end(); }

        bool hasFlagType(VariantFlag flag) const;

        std::unordered_set<size_t> flagTypes;
        std::vector<FieldValueVariant> variants;
    };

    VariantResolver(const FlagTable& flags, const IdentifierTable& ids);

    auto resolve(FieldValue& value) -> std::vector<FieldValueVariant>;

    auto operator()(const LiteralValue& val) const -> ValueVariantSet;
    auto operator()(const Identifier& id) const -> ValueVariantSet;
    auto operator()(const ListDeclaration& list) const -> ValueVariantSet;
    auto operator()(const ObjectDeclaration& obj) const -> ValueVariantSet;
    auto operator()(const MatchExpression& expr) const -> ValueVariantSet;

private:
    static bool isVariantOfSameFlag(const FieldValueVariant& a,
                                    const FieldValueVariant& b);
    static bool isVariantOfSameFlag(const VariantFlagSet& a,
                                    const VariantFlagSet& b);
    static void mergeFlags(VariantFlagSet& dst,
                           const VariantFlagSet& src);

    /**
     * Helper that inflates a variant value to a list of additional flag types.
     */
    auto generateVariants(const std::unordered_set<size_t>& requiredFlagTypes,
                          FieldValueVariant&& variant) const
        -> std::vector<FieldValueVariant>;

    const FlagTable& flagTable;
    const IdentifierTable& identifierTable;
};
