#pragma once

#include <string>
#include <vector>
#include <unordered_map>

#include "SyntaxElements.h"
#include "VariantFlagSet.h"

class FlagTable
{
public:
    /** @brief Enums are translated into these flag types */
    struct FlagDesc
    {
        FlagDesc(std::string name, std::vector<std::string> bits)
            : flagName(std::move(name)), flagBits(std::move(bits)) {}

        std::string flagName;
        std::vector<std::string> flagBits;
    };

    void registerFlagType(const EnumTypeDef& def);

    auto getRef(const std::string& flagName, const std::string& bit) const -> VariantFlag;
    auto getFlagType(size_t flagIndex) const -> std::string;
    auto getFlagBit(VariantFlag ref) const -> std::pair<std::string, std::string>;

    /**
     * @param VariantFlag flag
     *
     * @return size_t The number of flag bits in a flag type.
     */
    auto getNumFlagBits(VariantFlag flag) const -> size_t;

    auto makeFlagDescriptions() const -> std::vector<FlagDesc>;

private:
    struct FlagStorage
    {
        std::string name;
        std::vector<std::string> bits;
        std::unordered_map<std::string, size_t> bitsToIndices;
    };

    std::vector<FlagStorage> flags;
    std::unordered_map<std::string, size_t> flagsToIndices;
};

class FlagTypeCollector
{
public:
    auto collect(const std::vector<Stmt>& statements) -> FlagTable;

    void operator()(const ImportStmt&) {}
    void operator()(const TypeDef& def);
    void operator()(const FieldDefinition&);

    void operator()(const EnumTypeDef& def);

private:
    FlagTable table;
};
