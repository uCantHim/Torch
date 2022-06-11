#include "VariantResolver.h"

#include <ranges>
#include <algorithm>

#include "Exceptions.h"
#include "Util.h"
#include "IdentifierTable.h"



VariantResolver::ValueVariantSet::ValueVariantSet(std::vector<FieldValueVariant> _vars)
    :
    variants(std::move(_vars))
{
    for (const auto& var : this->variants) {
        for (const auto& flag : var.setFlags) {
            flagTypes.emplace(flag.flagId);
        }
    }
}

bool VariantResolver::ValueVariantSet::hasFlagType(VariantFlag flag) const
{
    return flagTypes.contains(flag.flagId);
}



VariantResolver::VariantResolver(const FlagTable& flags, const IdentifierTable& ids)
    :
    flagTable(flags),
    identifierTable(ids)
{
}

auto VariantResolver::resolve(FieldValue& value) -> std::vector<FieldValueVariant>
{
    return std::visit(*this, value).variants;
}

auto VariantResolver::operator()(const LiteralValue& val) const -> ValueVariantSet
{
    std::vector<FieldValueVariant> res;
    res.push_back({ .setFlags={}, .value=val });
    return res;
}

auto VariantResolver::operator()(const Identifier& id) const -> ValueVariantSet
{
    if (!identifierTable.has(id)) {
        throw InternalLogicError("Identifier \"" + id.name + "\" is not present in identifier table.");
    }

    const IdentifierValue& referenced = identifierTable.get(id);
    return std::visit(VariantVisitor{
        [this, &id](const ValueReference& ref) -> ValueVariantSet
        {
            assert(ref.referencedValue != nullptr);
            auto variants = std::visit(*this, *ref.referencedValue);
            for (auto& var : variants) {
                var.value = id;
            }
            return variants;
        },
        [](const TypeName&) -> ValueVariantSet {
            throw InternalLogicError("Tried to resolve variants on an identifier value that"
                                     " was a type name.");
        },
        [&id](const DataConstructor&) -> ValueVariantSet {
            return { { FieldValueVariant{ .setFlags={}, .value=id } } };
        },
    }, referenced);
}

auto VariantResolver::operator()(const ListDeclaration& list) const -> ValueVariantSet
{
    std::vector<FieldValueVariant> results;
    results.push_back({ {}, list });
    for (size_t i = 0; const auto& item : list.items)
    {
        auto variants = std::visit(*this, item);
        std::vector<FieldValueVariant> newResults;
        for (auto& list : results)
        {
            for (auto& var : variants)
            {
                auto copy = list;

                // We don't add items for which the same flag type would be
                // set to different flag bits
                if (isVariantOfSameFlag(copy, var)) {
                    continue;
                }

                auto& copyValue = std::get<ListDeclaration>(copy.value);
                copyValue.items.at(i) = var.value;  // Set item of current copy to variant value
                mergeFlags(copy.setFlags, var.setFlags);
                newResults.emplace_back(std::move(copy));
            }
        }

        if (!newResults.empty()) {
            std::swap(results, newResults);
        }
        ++i;
    }

    return results;
}

auto VariantResolver::operator()(const ObjectDeclaration& obj) const -> ValueVariantSet
{
    std::vector<FieldValueVariant> results;
    results.push_back({ {}, obj });

    for (size_t i = 0; const auto& [name, value] : obj.fields)
    {
        auto variants = std::visit(*this, *value);
        std::vector<FieldValueVariant> newResults;
        for (auto& obj : results)
        {
            for (auto& var : variants)
            {
                auto copy = obj;

                // We don't add items for which the same flag type would be
                // set to different flag bits
                if (isVariantOfSameFlag(copy, var)) {
                    continue;
                }

                auto& copyValue = std::get<ObjectDeclaration>(copy.value);
                copyValue.fields.at(i).value = std::make_shared<FieldValue>(var.value);
                mergeFlags(copy.setFlags, var.setFlags);
                newResults.emplace_back(std::move(copy));
            }
        }

        if (!newResults.empty()) {
            std::swap(results, newResults);
        }
        ++i;
    }

    return results;
}

auto VariantResolver::operator()(const MatchExpression& expr) const -> ValueVariantSet
{
    /**
     * Test if a flag set contains a specific flag bit.
     */
    auto hasSameBit = [](const VariantFlagSet& set, const VariantFlag& flag) -> bool
    {
        return std::find_if(
            set.begin(), set.end(),
            [&flag](auto&& other){
                return other.flagId == flag.flagId && other.flagBitId == flag.flagBitId;
            }
        ) != set.end();
    };

    const auto flagName = expr.matchedType.name;

    // First pass to collect all required flag types
    std::vector<ValueVariantSet> caseVariants;  // Collect all variants once
    std::unordered_set<size_t> newFlagTypes;
    for (const auto& opt : expr.cases)
    {
        const auto currentOptFlag = flagTable.getRef(flagName, opt.caseIdentifier.name);
        auto& vars = caseVariants.emplace_back(std::visit(*this, *opt.value));
        newFlagTypes.insert(vars.flagTypes.begin(), vars.flagTypes.end());
        newFlagTypes.erase(currentOptFlag.flagId);  // Don't contain the current flag
    }

    // Second pass to generate variants
    std::vector<FieldValueVariant> results;
    for (size_t i = 0; const auto& opt : expr.cases)
    {
        const auto currentOptFlag = flagTable.getRef(flagName, opt.caseIdentifier.name);
        auto& baseVariants = caseVariants.at(i++);

        if (baseVariants.flagTypes.contains(currentOptFlag.flagId))
        {
            // The case's subtree has already been matched on the current match expression's
            // flag. This case only allows the variants of the same flag type that have the same
            // *flag bit*.
            for (auto& var : baseVariants)
            {
                if (hasSameBit(var.setFlags, currentOptFlag))
                {
                    std::ranges::move(
                        generateVariants(newFlagTypes, std::move(var)),
                        std::back_inserter(results)
                    );
                }
            }
        }
        else {
            // The flag type of this match expression is new for the case's subtree.
            for (auto& var : baseVariants)
            {
                var.setFlags.emplace_back(currentOptFlag);
                std::ranges::move(generateVariants(newFlagTypes, std::move(var)),
                                  std::back_inserter(results));
            }
        }
    }

    return results;
}

auto VariantResolver::generateVariants(
    const std::unordered_set<size_t>& requiredFlagTypes,
    FieldValueVariant&& _variant) const
    -> std::vector<FieldValueVariant>
{
    auto hasFlagType = [](const VariantFlagSet& set, const VariantFlag& flag) -> bool
    {
        return std::find_if(
            set.begin(), set.end(),
            [&flag](auto&& other){ return other.flagId == flag.flagId; }
        ) != set.end();
    };

    std::vector<FieldValueVariant> variants{ std::move(_variant) };
    for (const size_t flagType : requiredFlagTypes)
    {
#ifndef NDEBUG  // Debug asserts
        for (auto& var : variants) {
            assert(var.setFlags == variants.front().setFlags
                   && "All variants of a field value must have the same flags set");
        }
#endif

        const size_t numBits = flagTable.getNumFlagBits({ flagType, 0 });

        std::vector<FieldValueVariant> newVariants;
        for (auto& var : variants)
        {
            // Don't add if subtree has already been matched on the flag
            if (hasFlagType(var.setFlags, { flagType, 0 })) continue;

            // The flag type is new for the subtree - variate it over all bits of the
            // new flag type
            for (size_t flagBit = 0; flagBit < numBits; ++flagBit)
            {
                const VariantFlag currentFlag{ flagType, flagBit };

                auto copy = var;
                copy.setFlags.emplace_back(currentFlag);
                newVariants.emplace_back(std::move(copy));
            }
        }
        if (!newVariants.empty()) {
            std::swap(variants, newVariants);
        }
    }

    return variants;
}

bool VariantResolver::isVariantOfSameFlag(const FieldValueVariant& a, const FieldValueVariant& b)
{
    return isVariantOfSameFlag(a.setFlags, b.setFlags);
}

bool VariantResolver::isVariantOfSameFlag(const VariantFlagSet& a, const VariantFlagSet& b)
{
    for (auto& fa : a)
    {
        for (auto& fb : b)
        {
            if (fa.flagId == fb.flagId) {
                return true;
            }
        }
    }
    return false;
}

void VariantResolver::mergeFlags(
    VariantFlagSet& dst,
    const VariantFlagSet& src)
{
    for (const auto& flag : src)
    {
        if (std::ranges::find(dst, flag) == dst.end()) {
            dst.emplace_back(flag);
        }
    }
}
