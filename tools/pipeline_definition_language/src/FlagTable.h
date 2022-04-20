#pragma once

#include <string>
#include <vector>
#include <unordered_map>

#include "SyntaxElements.h"
#include "CompileResult.h"

class FlagTable
{
public:
    struct FlagBitReference
    {
        size_t flagId;
        size_t flagBitId;

        inline bool operator==(const FlagBitReference& a) const {
            return flagId == a.flagId && flagBitId == a.flagBitId;
        }
    };

    void registerFlagType(const EnumTypeDef& def);

    auto getRef(const std::string& flagName, const std::string& bit) const -> FlagBitReference;
    auto getFlagBit(FlagBitReference ref) const -> std::pair<std::string, std::string>;

    auto getAllFlags() -> std::vector<FlagDesc>;

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
