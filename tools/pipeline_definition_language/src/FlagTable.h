#pragma once

#include <string>
#include <vector>
#include <unordered_map>

#include "SyntaxElements.h"

struct VariantFlag
{
    size_t flagId;
    size_t flagBitId;

    inline bool operator==(const VariantFlag& a) const {
        return flagId == a.flagId && flagBitId == a.flagBitId;
    }
};

namespace std
{
    template<>
    struct hash<VariantFlag>
    {
        auto operator()(const VariantFlag& f) const -> size_t
        {
            return hash<size_t>{}((f.flagId << (sizeof(size_t) / 2)) + f.flagBitId);
        }
    };
} // namespace std

class FlagTable
{
public:
    /** @brief Enums are translated into these flag types */
    struct FlagDesc
    {
        std::string flagName;
        std::vector<std::string> flagBits;
    };

    void registerFlagType(const EnumTypeDef& def);

    auto getRef(const std::string& flagName, const std::string& bit) const -> VariantFlag;
    auto getFlagType(size_t flagIndex) const -> std::string;
    auto getFlagBit(VariantFlag ref) const -> std::pair<std::string, std::string>;

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

    void operator()(const TypeDef& def);
    void operator()(const FieldDefinition&);

    void operator()(const EnumTypeDef& def);

private:
    FlagTable table;
};
