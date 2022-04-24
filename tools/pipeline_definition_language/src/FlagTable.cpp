#include "FlagTable.h"

#include "Exceptions.h"



void FlagTable::registerFlagType(const EnumTypeDef& def)
{
    const size_t flagId = flags.size();
    auto& flag = flags.emplace_back();
    flag.name = def.name;
    for (const auto& opt : def.options)
    {
        const size_t bitId = flag.bits.size();
        flag.bits.emplace_back(opt);
        flag.bitsToIndices.try_emplace(opt, bitId);
    }

    flagsToIndices.try_emplace(flag.name, flagId);
}

auto FlagTable::getRef(const std::string& flagName, const std::string& bit) const -> VariantFlag
{
    const size_t flagId = flagsToIndices.at(flagName);
    return { .flagId=flagId, .flagBitId=flags.at(flagId).bitsToIndices.at(bit) };
}

auto FlagTable::getFlagType(size_t flagIndex) const -> std::string
{
    return flags.at(flagIndex).name;
}

auto FlagTable::getFlagBit(VariantFlag ref) const -> std::pair<std::string, std::string>
{
    const auto& flag = flags.at(ref.flagId);
    return { flag.name, flag.bits.at(ref.flagBitId) };
}

auto FlagTable::makeFlagDescriptions() const -> std::vector<FlagDesc>
{
    std::vector<FlagDesc> result;
    for (const auto& flag : flags) {
        result.emplace_back(flag.name, flag.bits);
    }

    return result;
}



auto FlagTypeCollector::collect(const std::vector<Stmt>& statements) -> FlagTable
{
    for (const auto& stmt : statements)
    {
        std::visit(*this, stmt);
    }

    return std::move(table);
}

void FlagTypeCollector::operator()(const TypeDef& def)
{
    std::visit(*this, def);
}

void FlagTypeCollector::operator()(const FieldDefinition&)
{
    // Nothing
}

void FlagTypeCollector::operator()(const EnumTypeDef& def)
{
    table.registerFlagType(def);
}
