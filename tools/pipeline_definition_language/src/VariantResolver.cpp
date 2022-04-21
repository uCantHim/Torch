#include "VariantResolver.h"

#include <ranges>
#include <algorithm>

#include "Exceptions.h"
#include "IdentifierTable.h"



VariantResolver::VariantResolver(const FlagTable& flags, const IdentifierTable& ids)
    :
    flagTable(flags),
    identifierTable(ids)
{
}

auto VariantResolver::resolve(FieldValue& value) -> std::vector<FieldValueVariant>
{
    return std::visit(*this, value);
}

auto VariantResolver::operator()(const LiteralValue& val) const -> std::vector<FieldValueVariant>
{
    std::vector<FieldValueVariant> res;
    res.push_back({ .setFlags={}, .value=val });
    return res;
}

auto VariantResolver::operator()(const Identifier& id) const -> std::vector<FieldValueVariant>
{
    const FieldValue* referenced = identifierTable.get(id);
    if (referenced != nullptr) {
        return std::visit(*this, *referenced);
    }

    throw InternalLogicError("Identifier \"" + id.name + "\" is not present in identifier table.");
}

auto VariantResolver::operator()(const ObjectDeclaration& obj) const -> std::vector<FieldValueVariant>
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
                *copyValue.fields.at(i).value = std::move(var.value);
                mergeFlags(copy.setFlags, var.setFlags);
                newResults.emplace_back(std::move(copy));
            }
        }

        std::swap(results, newResults);
        ++i;
    }

    return results;
}

auto VariantResolver::operator()(const MatchExpression& expr) const -> std::vector<FieldValueVariant>
{
    const auto flagName = expr.matchedType.name;

    std::vector<FieldValueVariant> results;
    for (const auto& opt : expr.cases)
    {
        const auto currentOptFlag = flagTable.getRef(flagName, opt.caseIdentifier.name);
        auto values = std::visit(*this, *opt.value);
        for (auto& value : values)
        {
            // Add current flag if not already by match lower in the tree
            if (value.setFlags.end() == std::ranges::find(value.setFlags, currentOptFlag)) {
                value.setFlags.emplace_back(currentOptFlag);
            }

            results.emplace_back(std::move(value));
        }
    }
    return results;
}

bool VariantResolver::isVariantOfSameFlag(const FieldValueVariant& a, const FieldValueVariant& b)
{
    for (auto& fa : a.setFlags)
    {
        for (auto& fb : b.setFlags)
        {
            if (fa.flagId == fb.flagId) {
                return fa.flagBitId != fb.flagBitId;
            }
        }
    }
    return false;
}

void VariantResolver::mergeFlags(
    std::vector<FlagTable::FlagBitReference>& dst,
    const std::vector<FlagTable::FlagBitReference>& src)
{
    for (const auto& flag : src)
    {
        if (std::ranges::find(dst, flag) == dst.end()) {
            dst.emplace_back(flag);
        }
    }
}
